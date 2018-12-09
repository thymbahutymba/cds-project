#!/bin/sh

'
Start new client with 2 resources after first deallocation (10).
After the deallocation of two clients that had required 7 and 3 resources will 
be allocated two other client that require 5 and 3 resource, even if there are 
still 2 resources available these will not be allocated because there is high 
priority request pending.
'
/usr/bin/tmux new-session ./manager \; \
    splitw -h ./client 10 \;    `# first resource allocation` \
    splitw -h ./client 7 \;     `# second resource allocation` \
    select-pane -t 0 \; \
    splitw -v ./client 5 \;     `# third resource allocation` \
    splitw -v ./client 3 \;     `# second resource allocation` \
    select-pane -t 3 \; \
    splitw -v ./client 3 \;     `# third resource allocation` \
    splitw -v ./client 9 \;     `# fourth resource allocation` \
    select-pane -t 6 \; \
    splitw -v /bin/bash \;      `# ./client 2 fifth resource allocation` \
    select-layout tiled \; \
    select-pane -t 7 \; \
    attach