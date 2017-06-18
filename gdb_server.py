import socketserver
import time

def checksum(packet):
    sum = 0
    for value in packet:
        sum += value
        sum %= 256
    return format(sum, '02x').encode('utf-8')

class GDBRequestHandler(socketserver.BaseRequestHandler):
    def handle(self):
        while True:
            time.sleep(0.01)
            data = self.request.recv(1024)
            print('handle', data)
            if b'qSupported' in data:
                reply = b'PacketSize=119'
            elif b'vMustReplyEmpty' in data:
                reply = b''
            elif b'Hg0' in data:
                reply = b'OK'
            elif b'qTStatus' in data:
                reply = b'T0;tnotrun:0'
            elif b'qTfV' in data:
                reply = b''
            elif b'qTfP' in data:
                reply = b''
            elif b'qfThreadInfo' in data:
                reply = b'm00'
            elif b'qsThreadInfo' in data:
                reply = b'l'
            elif b'qAttached' in data:
                reply = b'0'
            elif b'Hc' in data:
                reply = b'OK'
            elif b'qC' in data:
                reply = b''
            elif b'?' in data:
                reply = b'S05'
            elif b'g' in data:
                # 32 general-purpose; sr; lo; hi; bad; cause; pc; 32 floating-point registers; fsr; fir; fp.
                reply = b'1000000000000008000000000000000000000000000000000000000000000000' \
                        b'2000000000000000000000000000000000000000000000000000000000000000' \
                        b'3000000000000000000000000000000000000000000000000000000000000000' \
                        b'4000000000000000000000000000000000000000000000000000000000000000' \
                        b'50000000' \
                        b'60000000' \
                        b'70000000' \
                        b'80000000' \
                        b'90000000' \
                        b'a0000000' \
                        b'b000000000000000000000000000000000000000000000000000000000000000' \
                        b'c000000000000000000000000000000000000000000000000000000000000000' \
                        b'd000000000000000000000000000000000000000000000000000000000000000' \
                        b'e000000000000000000000000000000000000000000000000000000000000000' \
                        b'f0000000' \
                        b'11000000' \
                        b'21000000'
            elif b'm' in data:
                reply = b'10203040'
            else:
                reply = None


            if reply is not None:
                reply = b'+$' + reply + b'#' + checksum(reply)
            else:
                reply = b'-'
            self.request.sendall(reply)
            print('sending:', reply)

class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

if __name__ == '__main__':
    server = ReusableTCPServer(('localhost', 6666), GDBRequestHandler, bind_and_activate=True)
    print('main')
    server.serve_forever()