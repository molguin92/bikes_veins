#include "veins/modules/application/bikes/BikeManeuver.h"

Define_Module(BikeManeuver);

void BikeManeuver::initialize(int stage)
{
	std::cout << "initializing...\n";
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
		prepareManeuver();
		break;
	}
}

void BikeManeuver::prepareManeuver()
{
	std::cout << "preparing...\n";
	if (myRole == CAR)
	{
		SimTime beginTime = SimTime(uniform(0.001, 1.0));
		selfbeacon = new cMessage();
		ping_interval = SimTime(0.1);
		scheduleAt(simTime() + ping_interval + beginTime, selfbeacon);
	}
	else if (myRole == BIKE)
		scheduleAt(simTime() + SimTime(0.1), new cMessage);
}

void BikeManeuver::finish()
{

	if (selfbeacon)
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
	else
		handleTurnWarning();
}

void BikeManeuver::handleSelfBeacon(cMessage *msg)
{
	//check current leader status
	FSM_Switch(carFSM)
	{
	case FSM_Exit(CAR_INIT):
	{
		FSM_Goto(carFSM, CAR_IDLE);
		scheduleAt(simTime() + ping_interval, selfbeacon);
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
	std::cout << "Handle turn warning or init msg...\n";
	FSM_Switch(bikeFSM)
	{
	case FSM_Exit(BS_INIT):
		FSM_Goto(bikeFSM, BS_GO);
		break;
	case FSM_Exit(BS_GO):
	{
		std::cout << "Should I brake?\n";
		if (traci->getDistance(
				traci->junction("cluster_0__0_0__2_0__4_0__6").getPosition(),
				mobility->getCurrentPosition(), false) <= 40)
		{
			//Warning! Car is turning.
			std::cout << "Braking\n";
			mobility->getVehicleCommandInterface()->setSpeed(0);
			FSM_Goto(bikeFSM, BS_STOP);
		}
		break;
	}
	case FSM_Exit(BS_STOP):
	{
		// Car done turning
		std::cout << "Going again\n";
		mobility->getVehicleCommandInterface()->setSpeed(20 / 3.6);
		FSM_Goto(bikeFSM, BS_GO);
	}
	}
}

void BikeManeuver::onData(WaveShortMessage *wsm)
{
	std::cout << "Got data!\n";
	//BaseWaveApplLayer::onData(wsm);
	if (myRole == BIKE && strcmp(wsm->getWsmData(), warning) == 0)
	{
		handleTurnWarning();
	}
	else if (strcmp(wsm->getWsmData(), warning) != 0)
	{
		std::cerr << "Unknown message.\n";
	}
}

void BikeManeuver::onBeacon(WaveShortMessage* wsm)
{
	//BaseWaveApplLayer::onBeacon(wsm);
}
