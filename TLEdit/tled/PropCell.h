//
//  PropCell.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 5/13/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef PropCell_h
#define PropCell_h

namespace Undo {
class PropCellBase {
public:
    WP_cord wp_x, wp_y;
    virtual ~PropCellBase() = 0;
    virtual void Snapshot (GraphicObject* g) = 0;
    virtual void Restore(GraphicObject* g) = 0;
    virtual PropCellBase* PostSnap(GraphicObject* g) = 0;
    
    void SnapWPpos(GraphicObject* g) {
        wp_x = g->wp_x;
        wp_y = g->wp_y;
    }
    
    void RestoreWPpos(GraphicObject* g) {
        g->MoveWP(wp_x, wp_y); // No-op if already in the right place.
    }
};

} //namespace Undo

#if !TLEDIT
template <class Derived>
class PropEditor {};

#else

template <typename Derived>  /* CRTP "Curiously Recurring Template Pattern", from ATLanta, apparently */
class PropEditor {
public:
    Undo::PropCellBase* PropCellCache;

    Undo::PropCellBase * CacheInitSnapshot() {
        PropCellCache = Derived::PropCell::CreatePropCell();
        PropCellCache->Snapshot(static_cast<Derived*>(this));
        return PropCellCache;
    }

    void DiscardPropCache() {
        delete TakeAndOwnCache();
    }
    
    Undo::PropCellBase* TakeAndOwnCache() {
        Undo::PropCellBase* val = PropCellCache;
        PropCellCache = nullptr;
        return val;
    }
    
    Undo::PropCellBase* PostSnap(GraphicObject* g) {
        Undo::PropCellBase* npc = Derived::PropCell::CreatePropCell();
        npc->Snapshot(g);
        return npc;
    }

};

template <typename DerivedCellType, typename GraphicObjectType>
class PropCellPCRTP : public Undo::PropCellBase { //P = "pseudo"
public:
    virtual Undo::PropCellBase* PostSnap (GraphicObject* g) {
        return ((GraphicObjectType*)g)->PostSnap(g);
    }

    static Undo::PropCellBase* CreatePropCell() {
        return new DerivedCellType();
    }
};


#endif

#endif /* PropCell_h */
