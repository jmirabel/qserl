class RodGui (object):
    def __init__ (self, rod, name, color, group=None):
        from gepetto.corbaserver import Client
        self.name = name
        self.gui = Client ()
        params = rod.parameters()
        self.gui.gui.addRod (name, color,
                params.radius, params.integrationTime, params.numNodes)
        if group is not None:
            self.gui.gui.addToGroup (name, group)

    def plot (self, state):
        from qserl.rod3d import displacementToTQ
        self.gui.gui.applyConfiguration (self.name, displacementToTQ (state.base()))
        for i in range(state.numNodes()):
            bMi = state.node(i)
            self.gui.gui.applyConfiguration (self.name + "_cap" + str(i), displacementToTQ (bMi))
        self.gui.gui.refresh ()
