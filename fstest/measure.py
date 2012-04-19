#!/usr/bin/python

import os, sys
import random

gluster_path = '/mnt/geo-sync2'
drbd_path = '/mnt/drbd1'
test_root = '/root'

def start_test(test, count, dst, src):
	try:
		pid = os.fork();
	except OSError, e:
		raise RuntimeError("Fork failed: %s [%d]" % (e.strerror, e.errno))
	if (pid != 0):
		try:
			if (src != None):
				os.execl(test_root + '/fstest.sh', test, count, dst, src)
			else:
				os.execl(test_root + '/fstest.sh', test, count, dst)
		except Exception, e:
			os._exit(255)

def usage():
	print('Usage: measure.py test size (gluster|drbd) [src]')
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) < 4):
		usage()
	
	test = sys.argv[1]
	count = sys.argv[2]
	
	if ((sys.argv[3] == 'gluster') or (sys.argv[3] == 'drbd')):
		dst = gluster_path + '/data.' + str(random.random())[4:] + '.out'
	else:
		usage()

	if (len(sys.argv) > 4):
		src = sys.argv[4]
	else
		src = None

	start_test(test, count, dst, src)
