#ifndef BATTLE_RAND_ENGINE_H
#define BATTLE_RAND_ENGINE_H
# include <iostream>

namespace engine
{
    class RandEngine
    {
    public:
        RandEngine(uint64_t seed) : m_Xn(seed) {}
        ~RandEngine() {}

        int RandBetween(int min,  int max) {
            if (min > max) {
                int temp = min;
                min = max;
                max = temp;
            }

            return min + (int)(1.0 * rand() / SR_M * (max - min) + 0.5);
        }

    private:
        uint32_t rand() {
            m_Xn = ((SR_A * m_Xn + SR_C) % SR_M);
            return m_Xn;
        }

    private:
        uint64_t m_Xn = 0;

        //算法数学模型 X(n+1) = (a * X(n) + c) % m ，参数选取参考gcc
        const static uint64_t SR_A = ((uint64_t)1103515245);
        const static uint64_t SR_C =((uint64_t)12345);
        const static uint64_t SR_M =((uint64_t)1<<32);
    };
}
#endif
