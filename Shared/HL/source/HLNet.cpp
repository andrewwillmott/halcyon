//
//  File:       HLNet.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLNet.h>

#include <CLMemory.h>
#include <CLSTL.h>

#ifdef HL_LIB_UV
    #include "../../../External/LibUV/uv.h"
#endif

#include "http_parser.h"

#include <stdio.h>
#include <unistd.h>

using namespace nCL;
using namespace nHL;

namespace
{

}


#ifdef HL_LIB_UV
namespace
{
    uv_timer_t sTimer;
    static int sTimerCount = 0;

    void TimerCallback(uv_timer_t* handle, int status)
    {
        printf("Timer: %d, status %d\n", sTimerCount++, status);
    }

    void RunJob(uv_work_t* request);
    void DoneJob(uv_work_t* request, int status);

    struct cJob
    {
        void DoWork()
        {
            sleep(3);
            printf("DID SOME WORK\n");
        }

        void OnFinish()
        {
            // uv_queue_work(loop, this, RunJob, DoneJob);
        }
    };

    struct cJobInternal :
        public cJob,
        public uv_work_t
    {
        static void RunJob (uv_work_t* request);
        static void DoneJob(uv_work_t* request, int status);
    };

    void cJobInternal::RunJob(uv_work_t* request)
    {
        static_cast<cJobInternal*>(request)->DoWork();
    }

    void cJobInternal::DoneJob(uv_work_t* request, int status)
    {
        if (status == 0)
            static_cast<cJobInternal*>(request)->OnFinish();
    }

    void OnResolved(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
    {
        if (status < 0)
        {
            fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status));
            return;
        }

        do
        {
            char addr[17] = {'\0'};
            uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
            fprintf(stderr, "Resolved to %s\n", addr);

            res = res->ai_next;
        }
        while (res);
    }

    void QueueJob(cJob* job)
    {
        uv_queue_work(uv_default_loop(), static_cast<cJobInternal*>(job), cJobInternal::RunJob, cJobInternal::DoneJob);
    }
}

void nHL::TimerTest()
{
    uv_loop_t* loop = uv_default_loop();

    uv_timer_init(loop, &sTimer);
    uv_timer_start(&sTimer, TimerCallback, 5000, 10000);
}

void nHL::ResolveName(const char* dnsName, const char* service)
{
    uv_loop_t* loop = uv_default_loop();

    struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    static uv_getaddrinfo_t resolver;
    int r = uv_getaddrinfo(loop, &resolver, OnResolved, dnsName,service, &hints);

    if (r < 0)
    {
        fprintf(stderr, "getaddrinfo call error %s\n", uv_err_name(r));
        return;
    }
}

void nHL::ShowInterfaces()
{
    char buf[512];
    uv_interface_address_t* info;
    int count;

    uv_interface_addresses(&info, &count);

    printf("Number of interfaces: %d\n", count);

    for (int i = count - 1; i >= 0; i--)
    {
        uv_interface_address_t interface = info[i];

        printf("Name: %s\n", interface.name);
        printf("  Internal? %s\n", interface.is_internal ? "Yes" : "No");
        
        if (interface.address.address4.sin_family == AF_INET)
        {
            uv_ip4_name(&interface.address.address4, buf, sizeof(buf));
            printf("  IPv4 address: %s\n", buf);
        }
        else if (interface.address.address4.sin_family == AF_INET6)
        {
            uv_ip6_name(&interface.address.address6, buf, sizeof(buf));
            printf("  IPv6 address: %s\n", buf);
        }

        printf("\n");
    }

    uv_free_interface_addresses(info, count);
}


void nHL::JobTest()
{
    static cJob runJob;

    QueueJob(&runJob);
}

namespace
{
    struct cHTTPHeader
    {
        string mField;
        string mValue;
    };

    enum tParseType
    {
        kHTTPNull,
        kHTTPHeaderField,
        kHTTPHeaderValue,
        kHTTPBody,
        kMaxHTTPParseTypes
    };

    struct cHTTPQuery
    {
        void Init();

        int OnMessage();

    protected:
        http_parser_settings    mHTTPSettings;

        http_parser             mHTTPParser;
        tParseType              mLastParseType = kHTTPNull;

        int                     mStatusCode = 0;
        vector<cHTTPHeader>     mHeaders;
        string                  mBody;

        // Parse callbacks
        static int OnMessageBegin   (http_parser* parser);
        static int OnMessageComplete(http_parser* parser);
        static int OnHeadersComplete(http_parser* parser);
        static int OnStatusComplete (http_parser* parser);
        static int OnHeaderField(http_parser* parser, const char *at, size_t length);
        static int OnHeaderValue(http_parser* parser, const char *at, size_t length);
        static int OnBody       (http_parser* parser, const char *at, size_t length);

    public:
        static void StreamAllocate(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf);
        static void StreamRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    };

    void cHTTPQuery::Init()
    {
        mHTTPSettings.on_message_begin      = OnMessageBegin;
        mHTTPSettings.on_status_complete    = OnStatusComplete;
        mHTTPSettings.on_header_field       = OnHeaderField;
        mHTTPSettings.on_header_value       = OnHeaderValue;
        mHTTPSettings.on_headers_complete   = OnHeadersComplete;
        mHTTPSettings.on_body               = OnBody;
        mHTTPSettings.on_message_complete   = OnMessageComplete;

        http_parser_init(&mHTTPParser, HTTP_RESPONSE);
        mHTTPParser.data = this;
    }

    int cHTTPQuery::OnMessage()
    {
        printf("Response, status: %d\n", mStatusCode);

        printf("Headers:\n");
        for (int i = 0, n = mHeaders.size(); i < n; i++)
            printf("  %s = %s\n", mHeaders[i].mField.c_str(), mHeaders[i].mValue.c_str());

        printf("Body:\n");
        printf("  %s\n", mBody.c_str());

        return 0;
    }

    // Callbacks
    int cHTTPQuery::OnMessageBegin(http_parser* parser)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);
        query->mHeaders.clear();
        query->mBody.clear();
        query->mLastParseType = kHTTPNull;
        return 0;
    }
    int cHTTPQuery::OnMessageComplete(http_parser* parser)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);
        return query->OnMessage();
    }
    int cHTTPQuery::OnHeadersComplete(http_parser* parser)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);
        query->mLastParseType = kHTTPNull;
        return 0;
    }
    int cHTTPQuery::OnStatusComplete(http_parser* parser)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);
        query->mStatusCode = parser->status_code;
        return 0;
    }

    int cHTTPQuery::OnHeaderField(http_parser* parser, const char *at, size_t length)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);

        if (query->mLastParseType != kHTTPHeaderField)
            query->mHeaders.push_back();
        query->mLastParseType = kHTTPHeaderField;

        query->mHeaders.back().mField.append(at, length);

        return 0;
    }
    int cHTTPQuery::OnHeaderValue(http_parser* parser, const char *at, size_t length)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);

        query->mLastParseType = kHTTPHeaderValue;
        query->mHeaders.back().mValue.append(at, length);

        return 0;
    }
    int cHTTPQuery::OnBody(http_parser* parser, const char *at, size_t length)
    {
        auto query = static_cast<cHTTPQuery*>(parser->data);

        query->mLastParseType = kHTTPBody;
        query->mBody.append(at, length);

        return 0;
    }

    void cHTTPQuery::StreamAllocate(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
    {
//        suggestedSize = 16;
        buf->base = (char*) Allocator(kNetworkAllocator)->Alloc(suggestedSize);
        buf->len  = suggestedSize;
    }

    void cHTTPQuery::StreamRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
    {
        auto query = static_cast<cHTTPQuery*>(stream->data);

        if (nread < 0)
        {
            if (nread != UV_EOF)
            {
                printf("read error: %s\n", uv_strerror(nread));
                uv_read_stop(stream);   // error, so stop reading.
            }
            else
            {
                int nparsed = http_parser_execute(&query->mHTTPParser, &query->mHTTPSettings, buf->base, 0);
            }

            uv_close((uv_handle_t*) stream, NULL);
        }
        else
        {
            int nparsed = http_parser_execute(&query->mHTTPParser, &query->mHTTPSettings, buf->base, nread);
        }

        if (buf->base)
            Allocator(kNetworkAllocator)->Free(buf->base);
    }

    static cHTTPQuery sHTTPQueryTest;
}

namespace
{
    static http_parser sHTTPParser;
    static http_parser_settings sHTTPSettings;


    int OnMessageBegin(http_parser* parser)
    {
        printf("HTTP message begin\n");
        return 0;
    }
    int OnMessageComplete(http_parser* parser)
    {
        printf("HTTP message end\n");
        return 0;
    }
    int OnHeadersComplete(http_parser* parser)
    {
        printf("HTTP headers end\n");
        return 0;
    }
    int OnStatusComplete(http_parser* parser)
    {
        printf("HTTP status end: %d\n", parser->status_code);
        return 0;
    }

    int OnHeaderField(http_parser* parser, const char *at, size_t length)
    {
        printf("FIELD: ");
        fwrite(at, 1, length, stdout);
        printf("\n");

        return 0;
    }
    int OnHeaderValue(http_parser* parser, const char *at, size_t length)
    {
        printf("VALUE: ");
        fwrite(at, 1, length, stdout);
        printf("\n");

        return 0;
    }
    int OnBody(http_parser* parser, const char *at, size_t length)
    {
        printf("BODY: ");
        fwrite(at, 1, length, stdout);
        printf("\n");

        return 0;
    }

    void InitHTTP()
    {
        sHTTPSettings.on_message_begin = OnMessageBegin;
        sHTTPSettings.on_status_complete = OnStatusComplete;
        sHTTPSettings.on_header_field = OnHeaderField;
        sHTTPSettings.on_header_value = OnHeaderValue;
        sHTTPSettings.on_headers_complete = OnHeadersComplete;
        sHTTPSettings.on_body = OnBody;
        sHTTPSettings.on_message_complete = OnMessageComplete;
        http_parser_init(&sHTTPParser, HTTP_RESPONSE);
    }


    static uv_shutdown_t sShutdownRequest;
    static uv_write_t sWriteRequest;

    void StreamDataWasWritten(uv_write_t* req, int status);
    void StreamWasShutdown(uv_shutdown_t* req, int status);

    void StreamAllocate(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
    {
        suggestedSize = 16;
        buf->base = (char*) Allocator(kNetworkAllocator)->Alloc(suggestedSize);
        buf->len  = suggestedSize;
    }

    void StreamRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
    {
        if (nread < 0)
        {
            if (nread != UV_EOF)
            {
                printf("read error: %s\n", uv_strerror(nread));
                uv_read_stop(stream);   // error, so stop reading.
            }
            else
            {
                printf("DONE READING\n");

                int nparsed = http_parser_execute(&sHTTPParser, &sHTTPSettings, buf->base, 0);
            }

            uv_close((uv_handle_t*) stream, NULL);
        }
        else
        {
//            printf("READ DATA:\n");
//            fwrite(buf->base, 1, nread, stdout);

            int nparsed = http_parser_execute(&sHTTPParser, &sHTTPSettings, buf->base, nread);

        #ifdef WRITE_SECOND
            const char* message = "HELLO!\n\r\n\r";
            uv_buf_t messageBuffer = uv_buf_init((char*) message, CL_SIZE(message));

            nread = uv_write(&sWriteRequest, stream, &messageBuffer, 1, StreamDataWasWritten);
            if (nread < 0)
                printf("uv_write error: %s\n", uv_strerror(nread));

            nread = uv_shutdown(&sShutdownRequest, stream, StreamWasShutdown);
        #endif
        }

        if (buf->base)
            Allocator(kNetworkAllocator)->Free(buf->base);
    }

    void StreamDataWasWritten(uv_write_t* req, int status)
    {
        if (status)
            printf("uv_write error: %s\n", uv_strerror(status));
        else
            printf("DATA WRITTEN\n");
    }

    void StreamWasShutdown(uv_shutdown_t* req, int status)
    {
        CL_ASSERT(status == 0);
        printf("DONE WRITING\n");
    }

    void OnTCPConnect(uv_connect_t* req, int status)
    {
        if (status < 0)
            return;

    #ifndef WRITE_SECOND
        uv_buf_t messageBuffer;
//  write_req = malloc(sizeof(uv_write_t));

        messageBuffer.base = (char*) "GET / HTTP/1.0\r\n"
             "Host: nyan.cat\r\n"
             "\r\n";
        messageBuffer.len = strlen(messageBuffer.base);

       status = uv_write   (&sWriteRequest, req->handle, &messageBuffer, 1, StreamDataWasWritten);
       status = uv_shutdown(&sShutdownRequest, req->handle, StreamWasShutdown);
    #endif

        InitHTTP();

//        status = uv_read_start(req->handle, StreamAllocate, StreamRead);

        sHTTPQueryTest.Init();
        req->handle->data = &sHTTPQueryTest;
        status = uv_read_start(req->handle, cHTTPQuery::StreamAllocate, cHTTPQuery::StreamRead);
    }
}

bool nHL::ConnectTest()
{
    uv_loop_t* loop = uv_default_loop();
    int err;

    static uv_tcp_t tcpHandle;
    err = uv_tcp_init(loop, &tcpHandle);
    if (err < 0)
        return false;

    sockaddr_in clientAddress;
    err = uv_ip4_addr("0.0.0.0", 0, &clientAddress);
    if (err < 0)
        return false;

    sockaddr_in socketAddress;

//    err = uv_ip4_addr("10.0.88.2", 80, &socketAddress);
    err = uv_ip4_addr("173.194.34.145", 80, &socketAddress);  // Google

//    err = uv_ip4_addr("127.0.0.1", 80, &socketAddress);

    if (err < 0)
        return false;

    static uv_connect_t connect;
    err = uv_tcp_connect(&connect, &tcpHandle, (const struct sockaddr*) &socketAddress, OnTCPConnect);
    if (err < 0)
        return false;

    return true;
}

#endif


bool cURL::SetURL(const char* url)
{
    mURL = url;

    http_parser_url parsedURL;

    int err = http_parser_parse_url(url, strlen(url), 0, &parsedURL);
    mLastField.clear();

    for (int i = 0; i < kMaxURLFields; i++)
        if (parsedURL.field_set & (1 << i))
        {
            mFields [i] = mURL.data() + parsedURL.field_data[i].off;
            mLengths[i] = parsedURL.field_data[i].len;
        }
        else
        {
            mFields [i] = 0;
            mLengths[i] = 0;
        }

    mPort = parsedURL.port;

    return err == 0;
}

void nHL::URLTest()
{
    cURL url;

    url.SetURL("scheme://username:password@domain:8080/path?query_string#fragment_id");

    for (int i = 0; i < kMaxURLFields; i++)
        if (url.HasField(tURLField(i)))
            printf("field %d = %s\n", i, url.Field(tURLField(i)));
}
