import os
import sys
import traci
import subprocess



PORT = 8813
sumoProcess = subprocess.Popen(
    [os.environ['SUMO_HOME'] + "/bin/sumo",
     "-c",
     "TAPASCologne-0.24.0/cologne.sumocfg",
     "--remote-port",
     str(PORT)],
    stdout=sys.stdout, stderr=sys.stderr)

traci.init(PORT)
step = 0
while step < 1000:
    traci.simulationStep()
    print traci.vehicle.getPosition("34652_34652_352_0")
    print traci.vehicle.getSpeed("34652_34652_352_0")

traci.close()
