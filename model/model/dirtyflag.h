#ifndef MODEL_DIRTYFLAG_H
#define MODEL_DIRTYFLAG_H

namespace model
{
    class Dirty
    {
    public:
        Dirty() : m_dirty(false) {}
        
        bool IsDirty() const {
            return m_dirty;
        }
        
        void SetDirty() const {
            m_dirty = true;
        }
        
        void SetClean() const {
            m_dirty = false;
        }
        
    private:
        mutable bool m_dirty;
    };
}

#endif