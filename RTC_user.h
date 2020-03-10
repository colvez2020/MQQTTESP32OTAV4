#ifndef RTCUSER_H
#define RTCUSER_H

 struct TIME_Assert
{
  boolean result;
};

void Setup_RTC(void);
void printDate(void);
void FullDatetochar(char* Cadena);
void Datetochar(char* Cadena);
void Timetochar(char* Cadena);
bool time_validar_seg(long *time_ref,long seg_segmento);
bool time_equal(int ME, int DD, int HH);

#endif
