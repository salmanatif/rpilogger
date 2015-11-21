#!/usr/bin/python -u

'''
    ================ 2 things in 1 for remote reboot ==================
    This is a simple socket server listening on a specified port for a
    pattern. If match, the system will be rebooted. Must be run as root.
    you can test it by: 
        telnet <ip> <port> 
        <pattern>
        
    Secondly, it checks the presense of the file <file>. If not present,
    reboot will be scheduled. Useful if ssh died, but you have ftp 
    access.
    
    author:     Daniel Piri
    web:        http://opendatalogger.com
    licence:    public GPL, (CC-BY-NC-SA), free software, non-commercial
        
'''
import socket
import sys
from thread import *
import os
from time import *
import threading

HOST = ''   # Symbolic name, meaning all available interfaces
PORT = 21099 # Arbitrary non-privileged port
_PATTERN = '4pd0aMxyOy57pvAh9lcIjh'
_FILE='delete_to_reboot'

def touch(path):
    with open(path, 'a'):
        os.utime(path, None)
        os.chmod(path,0666)

def checkfile():
    if not os.path.isfile(_FILE):
        touch(_FILE)
        print strftime("%F %T %z",localtime())+": [reboot] because of file"
        reboot()
    else:
        #repetitively check
        th=threading.Timer(60, checkfile)
        th.daemon = True
        th.start()

def reboot():
    os.system("shutdown -r +300 scheduled by the "+__file__)
        
#Function for handling connections. This will be used to create threads
def clientthread(conn):
    #Sending message to connected client
    #conn.send('Welcome\n') #intended not to send anything
         
    #Receiving from client
    data = conn.recv(1024)
    #Checking for pattern
    #import pdb; pdb.set_trace()
    if (data==_PATTERN+'\r\n'):
        print 'pattern match'
        conn.sendall('OK. rebooting.\r\n')
        conn.close()
        print strftime("%F %T %z",localtime())+": [reboot] because of socket req"        
        reboot()
    else:
        conn.close()

            

if __name__ == '__main__':
    
    try:
        checkfile()
        
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print 'Socket created'
         
        #Bind socket to local host and port
        try:
            s.bind((HOST, PORT))
        except socket.error as msg:
            print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
            sys.exit()
             
        print 'Socket bind complete'
         
        #Start listening on socket
        s.listen(10)
        print 'Socket now listening'
     
     
        #now keep talking with the client
        while True:
            #wait to accept a connection - blocking call
            conn, addr = s.accept()
            nowstr=strftime("%F %T %z",localtime())
            print nowstr+': connected with ' + addr[0] + ':' + str(addr[1])
             
            #start new thread takes 1st argument as a function name to be run, second is the tuple of arguments to the function.
            start_new_thread(clientthread ,(conn,))
         
        s.close()
    except (KeyboardInterrupt, SystemExit):
        print strftime("%F %T %z",localtime())+": Interrupt ?"
        sys.exit()
