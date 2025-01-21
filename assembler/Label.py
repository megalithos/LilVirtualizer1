import struct
class WaitObject:
    def __init__(self, waitForLabelName, compiledInstruction):
        self.waitForLabelName = waitForLabelName
        self.compiledInstruction = compiledInstruction

class Label:
    allLabels = []
    compiledInstructionsWaitingForAddressChange = []

    def __init__(self, labelName, relativeAddress = 0):
        self.labelName = labelName
        self.relativeAddress = relativeAddress
        Label.allLabels.append(self)

        toRemove = []
        for i in Label.compiledInstructionsWaitingForAddressChange:
            if (i.waitForLabelName == labelName):
                i.compiledInstruction.SetOperand1(struct.pack("<i", relativeAddress))
                toRemove.append(i)
        
        for i in toRemove:
            Label.compiledInstructionsWaitingForAddressChange.remove(i)


        print(f"Created new label, labelName: {labelName}, relativeAddress: {relativeAddress}")

    @staticmethod
    def GetLabel(labelName):
        for label in Label.allLabels:
            if (label.labelName == labelName):
                return label
        return None
    
    @staticmethod
    def WaitForLabel(labelName, compiledInstruction):
        waitObject = WaitObject(labelName, compiledInstruction)
        Label.compiledInstructionsWaitingForAddressChange.append(waitObject)