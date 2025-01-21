from globals import *
import struct
from Util import ExitWithError


class Variable:
    baseAddress = 0
    allVariables = []
    allVariableData = b''

    def __init__(self, name:str, data):
        self.name=name
        self.data=data
        self.relativeAddress = self.baseAddress
        Variable.baseAddress += len(data)
        Variable.allVariableData += data
        self.allVariables.append(self)
        print(f"Created variable \"{name}\" with data: {data}")
    
    @staticmethod
    def PrintVariables():
        for variable in Variable.allVariables:
            print(f"Name: {variable.name}, Value: {variable.data}, Address: {variable.relativeAddress}")
    
    @staticmethod
    def GetSizeOfData()->int:
        return len(Variable.allVariableData)

    @staticmethod
    def GetByName(name:str):
        for variable in Variable.allVariables:
            if (variable.name == name):
                return variable
        return None
    
    @staticmethod
    def GetData():
        """
        Returns:
            Data as byte array
        """
        return Variable.allVariableData

    @staticmethod
    def NewVariable(name:str, data):
        variableExistsAlready = False
        for variable in Variable.allVariables:
            if name == variable.name:
                ExitWithError("Variable has been already defined")
        Variable(name, data)                
