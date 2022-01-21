#encoding=utf-8
import socket as sk

addr_pl, port_pl = "143.89.192.104", 8080
addr_ps, port_ps = "143.89.192.16", 8080


UdpSocket = sk.socket(sk.AF_INET, sk.SOCK_DGRAM)
# TcpSocket = sk.socket(sk.AF_INET, sk.SOCK_STREAM)  # for TCP
UdpSocket.bind(("", 34234))
UdpSocket.sendto(bytes(b"message"), (addr_pl, port_pl))     
