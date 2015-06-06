//
//  File:       CLSystemInfo.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLSystemInfo.h>

#import <stdio.h>
#import <string.h>

#import <mach/mach_host.h>
#import <mach/task.h>
#import <sys/sysctl.h>

#include <CoreFoundation/CoreFoundation.h>

#ifdef CL_OSX
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

void printMemoryInfo()
{
    size_t length;
    int mib[2];

    printf("Memory Info\n");
    printf("-----------\n");

    uint32_t pagesize;
    mib[0] = CTL_HW;
    mib[1] = HW_PAGESIZE;
    length = sizeof(pagesize);
    if (sysctl(mib, 2, &pagesize, &length, NULL, 0) != 0)
    {
        perror("getting page size");
    }
    printf("Page size = %d bytes\n", pagesize);
    printf("\n");

    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;

    vm_statistics_data_t vmstat;
    if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count) != KERN_SUCCESS)
    {
        printf("Failed to get VM statistics.");
    }

    double total = vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count;
    double wired = vmstat.wire_count / total;
    double active = vmstat.active_count / total;
    double inactive = vmstat.inactive_count / total;
    double free = vmstat.free_count / total;

    printf("Total =    %8d pages\n", vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count);
    printf("\n");
    printf("Wired =    %8d MB\n", vmstat.wire_count     * pagesize / 1024 / 1024);
    printf("Active =   %8d MB\n", vmstat.active_count   * pagesize / 1024 / 1024);
    printf("Inactive = %8d MB\n", vmstat.inactive_count * pagesize / 1024 / 1024);
    printf("Free =     %8d MB\n", vmstat.free_count     * pagesize / 1024 / 1024);
    printf("\n");
    printf("Total =    %8d MB\n", (vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count) * (pagesize / 1024) / 1024);
    printf("\n");
    printf("Wired =    %0.2f %%\n", wired    * 100.0);
    printf("Active =   %0.2f %%\n", active   * 100.0);
    printf("Inactive = %0.2f %%\n", inactive * 100.0);
    printf("Free =     %0.2f %%\n", free     * 100.0);
    printf("\n");

    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    uint32_t result;
    length = sizeof(result);

    if (sysctl(mib, 2, &result, &length, NULL, 0) != 0)
        perror("getting physical memory");

    printf("Physical memory = %8u MB\n", result / 1024 / 1024);

    mib[0] = CTL_HW;
    mib[1] = HW_USERMEM;
    length = sizeof(result);

    if (sysctl(mib, 2, &result, &length, NULL, 0) != 0)
        perror("getting user memory");

    printf("User memory =     %8u MB\n", result / 1024 / 1024);
    printf("\n");
}

void printProcessorInfo()
{
    uint32_t result;
    size_t length;
    int mib[2];

    printf("Processor Info\n");
    printf("--------------\n");

    mib[0] = CTL_HW;
    mib[1] = HW_CPU_FREQ;
    length = sizeof(result);
    if (sysctl(mib, 2, &result, &length, NULL, 0) != 0)
    {
        perror("getting cpu frequency");
    }
    printf("CPU Frequency = %u MHz\n", result / 1000 / 1000);

    mib[0] = CTL_HW;
    mib[1] = HW_BUS_FREQ;
    length = sizeof(result);
    if (sysctl(mib, 2, &result, &length, NULL, 0) != 0)
    {
        perror("getting bus frequency");
    }
    printf("Bus Frequency = %u MHz\n", result / 1000 / 1000);
    printf("\n");
}

void printBatteryInfo()
{
#if 0 && defined(CL_OSX)
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();
    CFArrayRef sources = IOPSCopyPowerSourcesList(blob);

    CFDictionaryRef pSource = NULL;
    const void *psValue;

    int numOfSources = CFArrayGetCount(sources);

    if (numOfSources == 0)
    {
        perror("Error getting battery info");
        return;
    }

    printf("Battery Info\n");
    printf("------------\n");

    for (int i = 0 ; i < numOfSources ; i++)
    {
        pSource = IOPSGetPowerSourceDescription(blob, CFArrayGetValueAtIndex(sources, i));

        if (!pSource)
        {
            perror("Error getting battery info");
            continue;
        }

        psValue = (CFStringRef) CFDictionaryGetValue(pSource, CFSTR(kIOPSNameKey));

        int curCapacity = 0;
        int maxCapacity = 0;
        int percent;

        psValue = CFDictionaryGetValue(pSource, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberGetValue((CFNumberRef)psValue, kCFNumberSInt32Type, &curCapacity);

        psValue = CFDictionaryGetValue(pSource, CFSTR(kIOPSMaxCapacityKey));
        CFNumberGetValue((CFNumberRef)psValue, kCFNumberSInt32Type, &maxCapacity);

        percent = (int)((double)curCapacity/(double)maxCapacity * 100);
        
        printf ("powerSource %d of %d: percent: %d/%d = %d%%\n", i+1, CFArrayGetCount(sources), curCapacity, maxCapacity, percent);
        printf("\n");

    }
#endif

}

void nCL::GetSystemInfo()
{
    printMemoryInfo();
    printProcessorInfo();
    printBatteryInfo();
}

size_t nCL::UsedMemory()
{
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);

    kern_return_t kerr = task_info
    (
        mach_task_self(),
        TASK_BASIC_INFO,
        (task_info_t)&info,
        &size
    );

    if (kerr == KERN_SUCCESS)
        return info.resident_size;

    return 0;
}

float nCL::UsedMemoryInMB()
{
    return UsedMemory() / (1024.0f * 1024.0f);
}

size_t nCL::FreeMemory()
{
    mach_port_t            hostPort = mach_host_self();
    mach_msg_type_number_t hostSize = sizeof(vm_statistics_data_t) / sizeof(integer_t);

    vm_size_t pageSize;
    host_page_size(hostPort, &pageSize);

    vm_statistics_data_t vmStat;

    if (host_statistics(hostPort, HOST_VM_INFO, (host_info_t) &vmStat, &hostSize) != KERN_SUCCESS)
    {
        return ~0;
    }

    return vmStat.free_count * pageSize;
}

float nCL::FreeMemoryInMB()
{
    return FreeMemory() / (1024.0f * 1024.0f);
}

