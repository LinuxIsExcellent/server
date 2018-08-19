#ifndef ENGINE_VECTOR2_H
#define ENGINE_VECTOR2_H

namespace ms
{
    namespace engine
    {
        class Vector2
        {
        public:
            Vector2() {}
            Vector2(float x, float y) : m_x(x), m_y(y) {}
            virtual ~Vector2() {}

            float x() const {
                return m_x;
            }

            float y() const {
                return m_y;
            }

            void Set(float x, float y) {
                m_x = x;
                m_y = y;
            }

        public:
            //点乘
            float Dot(const Vector2& vt) {
                return m_x * vt.m_x + m_y * vt.m_y;
            }
            /* 向量积（叉乘）
             * 按照右手定则：返回值顺时针为正，逆时针为负，等于0表示两向量夹角为180or0度
             */
            float Cross(const Vector2& vt) {
                return m_x * vt.m_y - m_y * vt.m_x;
            }

        private:
            float m_x = .0f;
            float m_y = .0f;
        };
    }
}

#endif // VECTOR2_H
