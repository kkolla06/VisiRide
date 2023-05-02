#! /usr/bin/python
# a simple tcp server
import socket,os
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('', 12000))
sock.listen(1)
while True:
    connection,address = sock.accept()
    buf = connection.recv(25000)
    print(buf)
    connection.close()