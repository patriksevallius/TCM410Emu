from ctypes import *

if __name__ == '__main__':
    emulator = cdll.LoadLibrary('emulator.so')
    cpu = byref(emulator.cpu)
    emulator.initialize_emulator(cpu, create_string_buffer(b"fw.bin"))
    emulator.initialize_cpu(cpu, 0x9fc00000)
    emulator.register_callbacks()
    while True:
        emulator.execute(cpu)
