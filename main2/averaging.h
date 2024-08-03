#ifndef AVERAGING_H
#define AVERAGING_H

float averageReadings(float readings[], int size) {
    float sum = 0;
    for (int i = 0; i < size; i++) {
        sum += readings[i];
    }
    return sum / size;
}

#endif
