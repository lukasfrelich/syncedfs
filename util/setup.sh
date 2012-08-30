#!/bin/bash

# slave node first
ssh root@virtual-kvm /root/setup-$1.sh slave

/root/setup-$1.sh master
