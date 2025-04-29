env = None

try:
    Import("env")
    print("Current CLI targets", COMMAND_LINE_TARGETS)
    print("Current Build targets", BUILD_TARGETS)
except:
    pass
import json
import time
import serial

BAUD_RATE = 9600

class Proto:
    def __init__(self, timings, tolerance):
        self.timings = timings
        self.tolerance = tolerance

sync_bit = Proto([360, 11160, 0, 0, 0, 0, 0, 0], 0.3)
zero_bit = Proto([360, 1080, 360, 1080, 0, 0, 0, 0], 0.3)
one_bit =  Proto([360, 1080, 1080, 360, 0, 0, 0, 0], 0.3)


def upload_protocols(source, target, env):
    # print(env.Dump())
    protos = [sync_bit, zero_bit, one_bit]


    port = "/dev/ttyUSB0"
    if env:
        prot = env["UPLOAD_PORT"]


    stm32 = serial.Serial(port=port, baudrate=BAUD_RATE, timeout=.1)

    # go into setup mode
    stm32.write(int(1).to_bytes(1,'big'))
    stm32.write(int(len(protos)).to_bytes(1,'big'))    

    for p in protos:
        for v in p.timings:
            stm32.write(int(v).to_bytes(2, 'big'))
        stm32.write(int(p.tolerance * 255).to_bytes(1, 'big'))
    
    # enable capture mode
    # stm32.write(int(ord('s')).to_bytes(1, 'big'))
    stm32.close()

def enable_capture(source, target, env):
    # print(env.Dump())
    protos = [sync_bit, zero_bit, one_bit]


    port = "/dev/ttyUSB0"
    if env:
        prot = env["UPLOAD_PORT"]


    stm32 = serial.Serial(port=port, baudrate=BAUD_RATE, timeout=.1)

    # enable capture mode
    stm32.write(int(ord('s')).to_bytes(1, 'big'))
    stm32.close()


if __name__ == '__main__':
    upload_protocols(None, None, None)
else:
    pass
    # upload a fixed pattern set on the chip
    # env.AddPostAction("upload", upload_protocols)

    # start capture after uploading
    #env.AddPostAction("upload", enable_capture)



