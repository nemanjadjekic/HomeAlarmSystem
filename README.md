# HomeAlarmSystem
  Home alarm system simulation on Mikroe EASY AVR7
    environment. PORTB represents keyboard, where user
    can enter password. PORTC.0 is button, which starts 
    alarm system, when user leaves object, that should 
    be covered by alarm system. ADC conversion (PORTA.5)
    represents movement detection in secure are, and it
    triggers alarm, if there is breakin. PWM on PORTD.7
    and Timer 2 represent alarm light. LCD display has 
    classic usage and it is used to printout system 
    status, if object is locked or unlocked.
