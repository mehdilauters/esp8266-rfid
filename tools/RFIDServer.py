import socket
import threading
import sys

TAG_SIZE = 10

class RFIDServer(object):
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        print >>sys.stderr, 'starting up on %s port %s' % (self.host, self.port)

    def listen(self):
        self.sock.listen(5)
        while True:
          print >>sys.stderr, 'waiting for a connection'
          client, address = self.sock.accept()
          #client.settimeout(60)
          threading.Thread(target = self.listenToClient,args = (client,address)).start()
          
          
    def on_tag_received(self, tag):
      print "tag %s received"%tag
    
    def listenToClient(self, client, address):
        print >>sys.stderr, 'client connected:', address
        size = 1
        tag = ''
        while True:
          data = client.recv(size)
          if data:
            #print ("%c"%data).encode('hex')
            tag = '%s%s'%(tag,data)
            if len(tag) == TAG_SIZE:
              self.on_tag_received(tag)
              tag = ''
          else:
            client.close()
            break

if __name__ == "__main__":
  port_num = 6677
  RFIDServer('0.0.0.0',port_num).listen()