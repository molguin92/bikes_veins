#include "veins/modules/application/bikes_veins/BikeManeuver.h"

Define_Module(BikeManeuver);

void BikeManeuver::initialize(int stage)
{
	BaseWaveApplLayer::initialize(stage);

	switch (stage)
	{
	case 0:
		// name finite state machines
		carFSM.setName("carFSM");
		bikeFSM.setName("bikeFSM");

		// init everything

		mobility = Veins::TraCIMobilityAccess().get(getParentModule());
		traci = mobility->getCommandInterface();
		traciVehicle = mobility->getVehicleCommandInterface();
		break;
	case 1:
		//set our type!
		std::string strId = mobility->getExternalId();

		if (strId.substr(0, 5).compare("bikes") == 0)
		{
			// I am a bike
			myRole = BIKE;
		}
		else if (strId.substr(0, 7).compare("platoon") == 0)
		{
			// I am a car
			myRole = CAR;
		}
		else
		{
			std::cerr << "Wrong vehicle type: ";
			std::cerr << strId;
			std::cerr << "\n";
			exit(69);
		}
		break;

		prepareManeuver();
	}
}

void BikeManeuver::prepareManeuver()
{
	if (myRole == CAR)
	{
		SimTime beginTime = SimTime(uniform(0.001, 1.0));
		selfbeacon = new cMessage();
		ping_interval = SimTime(0.1);
		scheduleAt(simTime() + ping_interval + beginTime, selfbeacon);
	}
}

void BikeManeuver::finish()
{

	if (turnmsg)
	{
		cancelAndDelete(turnmsg);
		turnmsg = 0;
	}

	if(selfbeacon)
	{
		cancelAndDelete(selfbeacon);
		selfbeacon = 0;
	}

	BaseWaveApplLayer::finish();
}

void BikeManeuver::handleSelfMsg(cMessage * msg)
{
	BaseWaveApplLayer::handleSelfMsg(msg);

	if (msg == selfbeacon)
		handleSelfBeacon(msg);
}

void BikeManeuver::handleSelfBeacon(cMessage *msg)
{
	//check current leader status
	FSM_Switch(carFSM)
	{
	case FSM_Exit(CAR_INIT):
	{
		FSM_Goto(carFSM, CAR_IDLE);
		break;
	}
	case FSM_Exit(CAR_IDLE):
	{
		// self beacon
		if (msg == selfbeacon)
		{
			if (traci->getDistance(
					traci->junction("cluster_0__0_0__2_0__4_0__6").getPosition(),
					mobility->getCurrentPosition(), false) > 30)
			{
				scheduleAt(simTime() + ping_interval, selfbeacon);
				break;
			}

			t_channel channel = dataOnSch ? type_SCH : type_CCH;
			turnmsg = prepareWSM("data", dataLengthBits, channel, dataPriority,
					-1, 2);
			turnmsg->setWsmData(warning);
			sendWSM(turnmsg);
			scheduleAt(simTime() + ping_interval, selfbeacon);
			FSM_Goto(carFSM, CAR_TURNING);
		}
		break;
	}
	case FSM_Exit(CAR_TURNING):
	{
		// self beacon
		if (msg == selfbeacon)
		{
			if (traci->getDistance(
					traci->junction("cluster_0__0_0__2_0__4_0__6").getPosition(),
					mobility->getCurrentPosition(), false) < 30)
			{
				scheduleAt(simTime() + ping_interval, selfbeacon);
				break;
			}

			t_channel channel = dataOnSch ? type_SCH : type_CCH;
			turnmsg = prepareWSM("data", dataLengthBits, channel, dataPriority,
					-1, 2);
			turnmsg->setWsmData(warning);
			sendWSM(turnmsg);
			scheduleAt(simTime() + ping_interval, selfbeacon);
			FSM_Goto(carFSM, CAR_IDLE);
		}
		break;
	}
	}

}

void BikeManeuver::handleTurnWarning()
{
	//check current status
	FSM_Switch(bikeFSM)
	{
	case FSM_Exit(BS_GO):
	{
		if (traci->getDistance(
				traci->junction("cluster_0__0_0__2_0__4_0__6").getPosition(),
				mobility->getCurrentPosition(), false) <= 30)
		{
			//Warning! Car is turning.
			mobility->getVehicleCommandInterface()->setSpeed(0);
			FSM_Goto(bikeFSM, BS_STOP);
		}
		break;
	}
	case FSM_Exit(BS_STOP):
	{
		// Car done turning
		mobility->getVehicleCommandInterface()->setSpeed(20 / 3.6);
		FSM_Goto(bikeFSM, BS_GO);
	}
	}
}

void BikeManeuver::onData(WaveShortMessage *wsm)
{
	BaseWaveApplLayer::onData(wsm);
	if (myRole == BIKE && strcmp(wsm->getWsmData(), warning) == 0)
	{
		handleTurnWarning();
		delete(wsm);
	}
	else if (strcmp(wsm->getWsmData(), warning) != 0)
	{
		std::cerr << "Unknown message.\n";
	}
}

void BikeManeuver::onBeacon(WaveShortMessage* wsm)
{
	BaseWaveApplLayer::onBeacon(wsm);
}
