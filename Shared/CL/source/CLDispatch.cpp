//
//  File:       CLDispatch.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLDispatch.h>

#include <dispatch/dispatch.h>
#include <stdio.h>
#include <fcntl.h>

namespace
{

}






void DoJob(void* data, size_t index)
{
    printf("callback job %lu\n", index);
}

void DoAsyncJob(void* data)
{
    printf("async job\n");
}


const dispatch_source_vnode_flags_t kAllVNodeFlags =
  DISPATCH_VNODE_DELETE
| DISPATCH_VNODE_WRITE
| DISPATCH_VNODE_EXTEND
| DISPATCH_VNODE_ATTRIB
| DISPATCH_VNODE_LINK
| DISPATCH_VNODE_RENAME
| DISPATCH_VNODE_REVOKE;

void SourceCallback(dispatch_source_t source, void* context)
{
    dispatch_source_vnode_flags_t flags = dispatch_source_get_data(source);
    printf("Source callback (%s), flags = 0x%08lx\n", context ? context : "", flags);
}


void SubmitJob()
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
//    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    // or _HIGH/_LOW/_BACKGROUND
    // or, dispatch_get_main_queue(), and call dispatch_main() periodically


    dispatch_group_t group = dispatch_group_create();
    dispatch_object_t object = 0;


    dispatch_async_f(queue, 0, DoAsyncJob);
    int count = 16;

    dispatch_group_enter(group);
    dispatch_apply_f(count, queue, 0, DoJob);
    dispatch_group_leave(group);

    dispatch_apply(count, queue,
        ^(size_t index)
        {
            printf("block job %lu / %d\n", index, count);
        }
    );

    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    printf("All done.\n");


    // Experiment with sources...
    int watchFD = open("/tmp/a.txt", O_EVTONLY);
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE, watchFD, kAllVNodeFlags, queue);
    dispatch_source_set_event_handler(source,
        ^{ SourceCallback(source, dispatch_get_context(source)); }
    );
    dispatch_set_context(source, (void*) "context message");
    dispatch_resume(source);

}

void nCL::DispatchTest()
{
    SubmitJob();
}
