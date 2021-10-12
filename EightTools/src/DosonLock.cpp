#include "DosonLock.h"

DosonLock::DosonLock()
{
    initialized = false;
}

void DosonLock::begin( int index, char* mqttName, TCA6408* checkLockChip, TCA6408* setLockChip, Adafruit_NeoPixel* neoPixelInstance )
{
    _checkLockChip = checkLockChip;
    _setLockChip = setLockChip;
    _neoPixelInstance = neoPixelInstance;
    _lockNum = index;
    lockName = mqttName;

    _shouldOpen = false;
    _numLeds = 1;
    state = LOCK_UNKOWN;
    setState( LOCK_INIT );
}

void DosonLock::maintain()
{
    bool currentlyOpen = isOpen();

    if( !currentlyOpen && _shouldOpen )
    {
        Serial.print(lockName);
        Serial.print(" Should be unlocked! currentlyOpen: ");
        Serial.println(currentlyOpen);
        _shouldOpen = false;
        unlock();
        delay(100);
        lock();
    }

    uint8_t input_values = _checkLockChip->readInput(1);
    Serial.println(input_values,BIN);

    if( state == LOCK_INIT )
    {
        if( currentlyOpen )
        {
            setState( LOCK_OPEN );
        }
        else
        {
            setState( LOCK_AVAILABLE );
        }
    }
    else if( (state == LOCK_AVAILABLE || state == LOCK_CHECKEDOUT ) && currentlyOpen )
    {
        setState( LOCK_OPEN );
    }
    else if( state == LOCK_OPEN && !currentlyOpen )
    {
        Serial.printlnf("_isCheckedOut %d",_isCheckedOut);
        if( _isCheckedOut )
        {
            setState( LOCK_CHECKEDOUT );
        }
        else
        {
            setState( LOCK_AVAILABLE );
        }
    }
}

void DosonLock::lock()
{   
    //Serial.printlnf("lock %s", lockName);
    uint8_t output_values;
    _setLockChip->readByte(&output_values, TCA6408_OUTPUT);

    //Serial.print(output_values,BIN);
    output_values &= ~(1 << _lockNum); // &-operator clear a bit
    //Serial.print(" -> ");
    //Serial.println(output_values,BIN);

    _setLockChip->writeByte(output_values, TCA6408_OUTPUT);
}

void DosonLock::unlock()
{
    //Serial.printlnf("unlock %s", lockName);
    uint8_t output_values;
    _setLockChip->readByte(&output_values, TCA6408_OUTPUT);

    //Serial.print(output_values,BIN);
    output_values |= (1 << _lockNum); // |-operator clear a bit
    //Serial.print(" -> ");
    //Serial.println(output_values,BIN);

    _setLockChip->writeByte(output_values, TCA6408_OUTPUT);
}

bool DosonLock::isOpen()
{
    uint8_t input_values = _checkLockChip->readInput(1);
    uint8_t val = (input_values >> _lockNum) & 0x01; // Bitshift the other direction and & it with 1
    return val == 0;
}

void DosonLock::setNumLeds( int val )
{
    _numLeds = val;
    _neoPixelInstance->updateLength(_numLeds);
    isCheckedOut( false );
    if( val > 1 )
    {
        Serial.println("Unicorn cabinet!");
    }
}

void DosonLock::doOpen()
{
    _shouldOpen = true;
    Serial.printlnf("doOpen %d",_lockNum);
}

void DosonLock::isCheckedOut( bool val)
{
    _isCheckedOut = val;

    if( _numLeds > 1)
    {
        int r = random(0,35);
        int g = random(0,35);
        int b = random(0,35);
        // Serial.printlnf("Set pixel %d to %d %d %d",i,r,g,b);
        for(int i=0;i<_numLeds;i++)
        {
            _neoPixelInstance->setColor(i,g,r,b); 
        }
        _neoPixelInstance->show();
    }
    else if( isOpen() )
    {
        _neoPixelInstance->setPixelColor(0, 0,0,55); // blue
        _neoPixelInstance->show();
    }
    else if( !val )
    {
        _neoPixelInstance->setPixelColor(0, 55,0,0); // green
        _neoPixelInstance->show();
    }
    else
    {
        _neoPixelInstance->setPixelColor(0, 0,55,0); // red
        _neoPixelInstance->show();
    }
    
}

void DosonLock::setState( lockStates newState )
{
    // setPixelColor(0, g,r,b);
    /*
    Serial.print( "setState " );
    Serial.print( (int)state );
    Serial.print( " : " );
    Serial.println( (int)newState );
    */

    if( true ) // state != newState
    {
        if( newState == LOCK_INIT )
        {
            _neoPixelInstance->begin();
            _neoPixelInstance->show();
        }
        else if( newState == LOCK_AVAILABLE )
        {
            _neoPixelInstance->setPixelColor(0, 55,0,0); // green
            _neoPixelInstance->show();
            Serial.printlnf("LOCK_AVAILABLE Lock %s is closed. Num leds: %d",lockName, _numLeds);
        }
        else if( newState == LOCK_CHECKEDOUT)
        {
            _neoPixelInstance->setPixelColor(0, 0,55,0); // red
            _neoPixelInstance->show();
            Serial.printlnf("Lock %s LOCK_CHECKEDOUT",lockName);
        }
        else if( newState == LOCK_OPEN)
        {
            _neoPixelInstance->setPixelColor(0, 0,0,55); // blue
            _neoPixelInstance->show();
            Serial.printlnf("Lock %s is open",lockName);
        }
        else if( newState == FIRMWARE_PENDING )
        {
            _neoPixelInstance->setPixelColor(0, 0,55,55); // purple
            _neoPixelInstance->show();
        }
        else if( newState == CONNECTION_LOST )
        {
            _neoPixelInstance->setPixelColor(0, 55,55,0); // yellow
            _neoPixelInstance->show();
        }
        else if( newState == MQTT_CONNECTIING )
        {
            if( _lockNum % 2 == 0)
            {
                _neoPixelInstance->setPixelColor(0, 55,55,0); // yellow
            }
            else
            {
                _neoPixelInstance->setPixelColor(0, 155,155,0); // yellower!
            }
            
            _neoPixelInstance->show();
        }
        else if( newState == CONNECTION_RESTORED ) // use this to restore state after loosing connection or such
        {
            if( isOpen() )
            {
                newState = LOCK_OPEN;
            }
            else if ( _isCheckedOut )
            {
                newState = LOCK_CHECKEDOUT;
            }
            else
            {
                newState = LOCK_AVAILABLE;
            }
            // Do one recursive call to make LEDs redraw to new state
            setState(newState);
        }
        else
        {
            _neoPixelInstance->setPixelColor(0,0,0,0);
            _neoPixelInstance->show();
        }
    }
    state = newState;
}