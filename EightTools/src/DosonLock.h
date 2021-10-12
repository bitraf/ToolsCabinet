#ifndef __DOSONLOCK_H_
#define __DOSONLOCK_H_

#include "TCA6408.h"
#include "Adafruit_NeoPixel.h"

enum lockStates {
    LOCK_UNKOWN,
    LOCK_INIT,
    LOCK_OPEN,
    LOCK_AVAILABLE,
    LOCK_CHECKEDOUT,
    FIRMWARE_PENDING,
    CONNECTION_LOST,
    CONNECTION_RESTORED,
    MQTT_CONNECTIING
};

class DosonLock {
    public:
        lockStates state;
    private:
        void checkState();
        void lock();
        void unlock();
        TCA6408* _checkLockChip;
        TCA6408* _setLockChip;
        Adafruit_NeoPixel* _neoPixelInstance;
        int _lockNum;
        bool _shouldOpen;
        bool _isCheckedOut;
        int _numLeds;
        boolean initialized;
        char* lockName;
    public:
        DosonLock();
        void begin( int index, char* mqttName, TCA6408* checkLockChip, TCA6408* setLockChip, Adafruit_NeoPixel* neoPixelInstance );
        void maintain();
        bool isOpen();
        void setNumLeds( int val );
        void isCheckedOut( bool val );
        void doOpen();
        void refresh();
        void setState( lockStates state );
};

#endif  // __DOSONLOCK_H_