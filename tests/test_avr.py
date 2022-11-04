import unittest
import avr


class TestAVR(unittest.TestCase):
    def test_registers(self):
        avr1 = avr.new()
        for register in range(0,32):
            for value in range(0,256):
                avr1.set_register(register, value)
                self.assertEqual(avr1.get_register(register), value)

    def test_sreg(self):
        avr1 = avr.new()
        for value in range(0,256):
            avr1.set_sreg(value)
            self.assertEqual(avr1.get_sreg(), value)

    def test_memory(self):
        avr1 = avr.new()

        for memory in range(avr1.get_program_memory_size()):
            self.assertEqual(0, avr1.get_program_memory(memory))

        for register in [0,412,avr1.get_program_memory_size()-1]:
            for value in [0, 255, pow(2,16)-1]:
                set_memory = avr1.set_program_memory(value, register)
                get = avr1.get_program_memory(register)
                #print("Register: ", register, " Set: ", set_memory, " Get: ", get)
                self.assertEqual(get, value)


    def test_sreg_instructions(self):
        avr1 = avr.new()

        program_counter = 0

        # BSET
        for i in range(0, 8):
            avr1.set_program_memory(int('100101000' + '{0:03b}'.format(i) + '1000', 2), program_counter)
            program_counter += 1

        cache = 0
        for i in range(0, 8):
            #print("Program Counter ", avr1.get_program_counter())
            avr1.run_next_instruction()
            cache += pow(2,i)
            self.assertEqual(cache, avr1.get_sreg())
            #print("Python SREG: ", avr1.get_sreg())

        # BCLR
        for i in range(0,8):
            avr1.set_program_memory(int('100101001'+'{0:03b}'.format(i)+'1000', 2), program_counter)
            program_counter += 1

        for i in range(0,8):
            #print("Program Counter ", avr1.get_program_counter())
            avr1.run_next_instruction()
            #print("Python SREG: ", avr1.get_sreg())
            cache -= pow(2,i)
            self.assertEqual(cache, avr1.get_sreg())

    def test_adc_add(self):
        avr1 = avr.new()
        avr1.set_register(0, 1)
        for i in range(0,32):
            avr1.set_program_memory(int('0001110000000000', 2), i)
            avr1.run_next_instruction()
            print("Register: {0:08b}".format(avr1.get_register(0)))
            print("{0:08b}".format(avr1.get_sreg()))


    def test_bld(self):
        avr1 = avr.new()
        avr1.set_sreg(int('01000000',2))
        avr1.set_program_memory(int('1111100000000000', 2), 0)
        avr1.set_program_memory(int('1111100000000001', 2), 1)
        avr1.run_next_instruction()
        print("Register: {0:08b}".format(avr1.get_register(0)))
        avr1.run_next_instruction()
        print("Register: {0:08b}".format(avr1.get_register(0)))
        avr1.run_next_instruction()


    def test_brbc(self):
        avr1 = avr.new()
        avr1.set_sreg(int('01000000',2))
        avr1.set_program_memory(int('1111011111111000', 2), 0)
        avr1.run_next_instruction()
        print("Register: {0:08b}".format(avr1.get_register(0)))
        avr1.run_next_instruction()
        print("Register: {0:08b}".format(avr1.get_register(0)))
        avr1.run_next_instruction()

    def dtest_run_all_instructions(self):
        instructions = ['0001110000000000',
                        '0000110000000000',
                        '0010000000000000',
                        '0111000000000000',
                        '1001010000000101',
                        '1001010010001000',
                        '1111100000000000',
                        '1111010000000000',
                        '1111000000000000',
                        '1111010000000000',
                        '1111000000000000',
                        '1111000000000001',
                        '1111010000000100',
                        '1111010000000101',
                        '1111000000000101',
                        '1111010000000111',
                        '1111000000000111',
                        '1111000000000000',
                        '1111000000000100',
                        '1111000000000010',
                        '1111010000000001',
                        '1111010000000010',
                        '1111010000000000',
                        '1111010000000100',
                        '1111000000000110',
                        '1111010000000011',
                        '1111000000000011',
                        '1001010000001000',
                        '1111101000000000',
                        '1001100000000000',
                        '0111000000000000',
                        '1001010010001000',
                        '1001010011011000',
                        '1001010011111000',
                        '1001010010101000',
                        '0010010000000000',
                        '1001010011001000',
                        '1001010011101000',
                        '1001010010111000',
                        '1001010010011000',
                        '1001010000000000',
                        '0001010000000000',
                        '0000010000000000',
                        '0011000000000000',
                        '0001000000000000',
                        '1001010000001010',
                        '0010010000000000',
                        '1011000000000000',
                        '1001010000000011',
                        '1001001000000100',
                        '1001001000000101',
                        '1001001000000111',
                        '1110000000000000',
                        '0000110000000000',
                        '1001010000000110',
                        '0010110000000000',
                        '1001010000000001',
                        '0000000000000000',
                        '0010100000000000',
                        '0110000000000000',
                        '1011100000000000',
                        '1100000000000000',
                        '0001110000000000',
                        '1001010000000111',
                        '0000100000000000',
                        '0100000000000000',
                        '1001101000000000',
                        '1001100100000000',
                        '1001101100000000',
                        '0110000000000000',
                        '1111110000000000',
                        '1111111000000000',
                        '1001010000001000',
                        '1001010001011000',
                        '1001010001111000',
                        '1001010000111000',
                        '1110111100001111',
                        '1001010001001000',
                        '1001010001101000',
                        '1001010000111000',
                        '1001010000011000',
                        '1001010110001000',
                        '0001100000000000',
                        '0101000000000000',
                        '1001010000000010',
                        '0010000000000000']
        avr1 = avr.new()
        program_counter = 0
        for instruction in instructions:
            avr1.set_program_memory(int(instruction, 2), program_counter)
            program_counter += 1

        for i in range(0,program_counter):
            avr1.run_next_instruction()

if __name__ == '__main__':
    unittest.main()
