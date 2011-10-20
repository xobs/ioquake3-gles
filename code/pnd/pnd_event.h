#ifndef PND_EVENT_H
#define PND_EVENT_H

void PND_Setup_Controls( void );
void PND_Close_Controls( void );

void PND_SendAllEvents( void );
void PND_SendKeyEvents( void );
void PND_SendRelEvents( void );
void PND_SendAbsEvents( void );

#endif
