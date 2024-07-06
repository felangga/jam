#include <ESP8266WiFi.h>
#include "utils.h"

int mapHourToPWM(int hour) {
  int value = 0;
  // The value calibrated based on the dial display
  switch (hour) {
    case 1:
      value = 30;
      break;
    case 2:
      value = 50;
      break;
    case 3:
      value = 70;
      break;
    case 4:
      value = 85;
      break;
    case 5:
      value = 100;
      break;
    case 6:
      value = 115;
      break;
    case 7:
      value = 130;
      break;
    case 8:
      value = 141;
      break;
    case 9:
      value = 155;
      break;
    case 10:
      value = 166;
      break;
    case 11:
      value = 177;
      break;
    case 12:
      value = 190;
      break;
  }

  return value;
}

int mapMinuteToPWM(int minute) {
  if (minute >= 0 && minute <= 10) {
    return map(minute, 0, 10, 0, 55);
  }
  if (minute >= 11 && minute <= 20) {
    return map(minute, 11, 20, 56, 97);
  }
  if (minute >= 21 && minute <= 30) {
    return map(minute, 21, 30, 98, 128);
  }
  if (minute >= 31 && minute <= 40) {
    return map(minute, 31, 40, 129, 153);
  }
  if (minute >= 41 && minute <= 59) {
    return map(minute, 41, 59, 154, 195);
  }

  return 0;
}


int mapSecondToPWM(int second) {
  if (second >= 0 && second <= 10) {
    return map(second, 0, 10, 0, 55);
  }
  if (second >= 11 && second <= 20) {
    return map(second, 11, 20, 56, 97);
  }
  if (second >= 21 && second <= 30) {
    return map(second, 21, 30, 98, 128);
  }
  if (second >= 31 && second <= 40) {
    return map(second, 31, 40, 129, 153);
  }
  if (second >= 41 && second <= 59) {
    return map(second, 41, 59, 154, 195);
  }

  return 0;
}