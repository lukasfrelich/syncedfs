#!/usr/bin/python

import os, sys
gluster_path = '/mnt/geo-sync2'
drbd_path = '/mnt/drbd1'

def start_test(self, test, size, src, dst):
	try:
		pid = os.fork();
	except OSError, e:
		raise RuntimeError("Fork failed: %s [%d]" % (e.strerror, e.errno))
	if (pid != 0):
		try:
			os.execl('/root/fstest.sh', 'seqwrite', )
		except Exception, e:
			os._exit(255)
	

if __name__ == '__main__':
	if (len(sys.argv) < 4):
		print('Not enough arguments')
		sys.exit(-1)
	
	test = sys.argv[1]
	
