import struct

class FileHeader:
    fileHeaderMagic = ""
    sizeOfData = 0
    pointerToData = 0
    sizeOfCode = 0
    pointerToCode = 0
    
    @staticmethod
    def SetDataPointer():
        FileHeader.pointerToData = len(FileHeader.fileHeaderMagic) + 8 + 8 # 4 for sizeOfData and 4 for pointerToData, 4 for sizeOfCode and 4 for pointerToCode

    @staticmethod
    def SetCodePointer():
        FileHeader.pointerToCode = len(FileHeader.fileHeaderMagic) + 8 + 8 + FileHeader.sizeOfData # 4 for sizeOfData and 4 for pointerToData, 4 for sizeOfCode and 4 for pointerToCode
        print(f"Codepointer is {FileHeader.pointerToData}")

    @staticmethod
    def ToBytes():
        return struct.pack(f'{len(FileHeader.fileHeaderMagic)}sii', FileHeader.fileHeaderMagic.encode(), FileHeader.sizeOfData, FileHeader.sizeOfCode)