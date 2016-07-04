#ifndef BikeManeuver_H_
#define BikeManeuver_H_

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
// #include "veins/modules/application/messages/BikesMessage_m.h"

class BikeManeuver: public BaseWaveApplLayer
{
protected:

	Veins::TraCIMobility* mobility;
	Veins::TraCICommandInterface *traci;
	Veins::TraCICommandInterface::Vehicle *traciVehicle;

	enum ROLE
	{
		CAR, BIKE
	};

	typedef enum _CAR_STATES
	{
		CAR_INIT = 0, CAR_IDLE = FSM_Steady(1), CAR_TURNING = FSM_Steady(2)
	} CAR_STATES;

	typedef enum _BIKE_STATES
	{
		BS_GO = FSM_Steady(1), BS_STOP = FSM_Steady(2)
	} BIKE_STATES;

	enum CAR_MSGS
	{
		MSG_TURNING = 0, MSG_DONE_TURNING = 1
	};

	cFSM carFSM, bikeFSM;
	ROLE myRole;
	int position;
	cMessage *selfbeacon;
	WaveShortMessage *turnmsg;
	SimTime ping_interval;
	const char *warning = "WARNING";

public:
	virtual void initialize(int stage);
	virtual void finish();
	BikeManeuver()
	{
		selfbeacon = 0;
		turnmsg = 0;
	}

protected:
	virtual void onBeacon(WaveShortMessage* wsm);
	virtual void onData(WaveShortMessage* wsm);

	virtual void handleSelfMsg(cMessage *msg);
	void prepareManeuver();

	void handleSelfBeacon(cMessage *msg);
	void handleTurnWarning();
};

#endif
