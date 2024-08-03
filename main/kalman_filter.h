#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

class KalmanFilter {
public:
    KalmanFilter(float processNoise, float measurementNoise, float estimationError, float value) {
        Q = processNoise;
        R = measurementNoise;
        P = estimationError;
        X = value;
    }

    float update(float measurement) {
        // Prediction 
        P = P + Q;

        // Measurement 
        K = P / (P + R);
        X = X + K * (measurement - X);
        P = (1 - K) * P;

        return X;
    }

private:
    float Q; // Process noise covariance
    float R; // Measurement noise covariance
    float P; // Estimation error covariance
    float K; // Kalman gain
    float X; // Data
};

#endif
