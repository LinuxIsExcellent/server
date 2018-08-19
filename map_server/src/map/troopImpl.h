#ifndef MAP_TROOP_IMPL_H
#define MAP_TROOP_IMPL_H
namespace ms
{
    namespace map
    {
        class Troop;
        class Unit;
        class ResNode;
        class CampFixed;
        class CampTemp;
        class TroopImpl
        {
        public:
            TroopImpl(Troop *troop) : m_troop(troop) {}
            ~TroopImpl() {}
            
            //攻击失败
            virtual void OnAttackFailed(Unit* unit);
            //攻击成功
            virtual void OnAttackWin(Unit* unit) {}
            //防守失败
            virtual void OnDefenceFailed(Unit* unit);
            //防守成功
            virtual void OnDefenceWin(Unit* unit) {}
            
            
            virtual bool OnReachProc(Unit* to) = 0;
            virtual void OnLeave();
            virtual void OnArriveHome();
            virtual void OnBack();
            //到达失效
            virtual void OnReachInvalid();
            // 到达返回(目标失去，自己迁城)
            virtual void OnMarchBack();
            
            virtual void FinishDeserialize();
            
        protected:

            void OnArriveCamp();
            void OnArriveCastle();
           
            CampTemp* CampingTemp(bool isSetup = false);
            bool CampingFixed();
            bool Occupy(CampFixed* campFixed);

        protected:
            
            Troop *m_troop;
        };
        
        //默认
        class DefaultTroopImpl : public TroopImpl
        {
        public:
            DefaultTroopImpl(Troop *troop)
            :TroopImpl(troop){}

            virtual bool OnReachProc(Unit* to) override;
        };


        //采集
        class TroopToRes : public TroopImpl
        {
        public:
            TroopToRes(Troop *troop)
            :TroopImpl(troop){}
            
            void Gathering(ResNode* res);
            void ReCalculateGatherTime();
            
             //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            //防守成功
            virtual void OnDefenceWin(Unit* unit) override;

            virtual bool OnReachProc(Unit* to) override;
            virtual void FinishDeserialize() override;
        };

        //打怪
        class TroopToMonster : public TroopImpl
        {
        public:
            TroopToMonster(Troop *troop)
            :TroopImpl(troop){}
            
             //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            virtual bool OnReachProc(Unit* to) override;
            virtual void OnReachInvalid() override;
        };

        //打怪
        class TroopToWorldBoss : public TroopImpl
        {
        public:
            TroopToWorldBoss(Troop *troop)
            :TroopImpl(troop){}
            
             //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            virtual bool OnReachProc(Unit* to) override;
            virtual void OnReachInvalid() override;
        };

        //名城
        class FamousCity;
        namespace tpl {
            struct CityPatrolEventTpl;
        };
        class TroopToCity : public TroopImpl
        {
        public:
            TroopToCity(Troop *troop)
            :TroopImpl(troop){}
            
            //防守失败
            virtual void OnDefenceFailed(Unit* unit) override;
             //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            
            virtual bool OnReachProc(Unit* to) override;

        protected:
            bool OnReachMyCity(FamousCity* city);
            bool OnReachOtherCity(FamousCity* city);

            void ProcPatrolEvent(FamousCity* city);

        protected:
//             std::vector<>;
        };

        //玩家
        class Castle;
        class TroopToCastle : public TroopImpl
        {
        public:
            TroopToCastle(Troop *troop)
            :TroopImpl(troop){}
            
            //防守失败
            virtual void OnDefenceFailed(Unit* unit) override;
            //攻击成功
            virtual void OnAttackWin(Unit* unit) override;

            bool OnReachAllianceCastle(Castle* castle);
            
            virtual bool OnReachProc(Unit* to) override;
            virtual void FinishDeserialize() override;
        };

        //驻扎
        class TroopToCampTemp : public TroopImpl
        {
        public:
            TroopToCampTemp(Troop *troop)
            :TroopImpl(troop){}

            //攻击成功
            virtual void OnAttackWin(Unit* unit) override;   
            virtual bool OnReachProc(Unit* to) override;
            virtual void FinishDeserialize() override;
        private: 
            CampTemp* m_campTemp = nullptr;
        };

        //行营
        class CampFixed;
        class TroopToCampFixed : public TroopImpl
        {
        public:
            TroopToCampFixed(Troop* troop)
                : TroopImpl(troop) {}

//             void RemoveParentCamp();

            //攻击失败
            virtual void OnAttackFailed(Unit* unit);
            //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            //防守失败
            virtual void OnDefenceFailed(Unit* unit) override;

            virtual bool OnReachProc(Unit* to) override;
            virtual void OnReachInvalid() override;

            bool OnReachAllianceCamp (CampFixed* camp);

        protected:

            void OnCreateCampFixedFailed();
        };

        //侦查
        class TroopToScout : public TroopImpl
        {
        public:
            TroopToScout(Troop *troop)
            :TroopImpl(troop){}
            
            bool Scout(Unit* unit);
            
            virtual bool OnReachProc(Unit* to) override;
        };

        //援助
        class TroopToReinforcements : public TroopImpl
        {
        public:
            TroopToReinforcements(Troop *troop)
            :TroopImpl(troop){}
            
            virtual bool OnReachProc(Unit* to) override;
            virtual void FinishDeserialize() override;
            virtual void OnBack() override;
        };


        //名城
        class Catapult;
        class TroopToCatapult : public TroopImpl
        {
        public:
            TroopToCatapult(Troop *troop)
            :TroopImpl(troop){}
                
            // //攻击失败
            // virtual void OnAttackFailed(Unit* unit);
            //攻击成功
            virtual void OnAttackWin(Unit* unit) override;
            //防守失败
            virtual void OnDefenceFailed(Unit* unit) override;

            virtual bool OnReachProc(Unit* to) override;
            virtual void OnReachInvalid() override;

        protected:
            bool OnReachAllianceCatapult(Catapult* cataputl);
            bool OnReachOtherCatapult(Catapult* cataputl);

        };

        class TroopToTransport : public TroopImpl
        {
        public:
            TroopToTransport(Troop *troop)
            :TroopImpl(troop){}
            
            virtual bool OnReachProc(Unit* to) override;
            virtual void OnBack() override;
            virtual void OnMarchBack() override;
        };

    }
}
#endif