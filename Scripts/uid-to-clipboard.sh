#!/bin/sh
perl -e 'printf("0x%08x", time() - 1356998400);' | pbcopy
