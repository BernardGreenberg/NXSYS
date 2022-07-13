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
    WPPOINT WpPoint() {return WPPOINT(wp_x, wp_y);}
    virtual ~PropCellBase() = 0;
    virtual void Snapshot (GraphicObject* g) = 0;
    virtual void Restore(GraphicObject* g) = 0;
    virtual PropCellBase* SnapshotNow(GraphicObject* g) = 0;
    
    void SnapWPpos(GraphicObject* g) {
        wp_x = g->wp_x;
        wp_y = g->wp_y;
    }
    
    void RestoreWPpos(GraphicObject* g) {
        g->MoveWP(wp_x, wp_y); // No-op if already in the right place.
    }
};

} //namespace Undo

#include <memory>

template <typename Derived>  /* CRTP "Curiously Recurring Template Pattern", from ATLanta, apparently */
class PropEditor {
private:
    std::unique_ptr<Undo::PropCellBase> PropCellCache;
public:

    /* Needed in BranchSwap cancel q.v. */
    void RestorePropCache() {
        PropCellCache->Restore(static_cast<Derived*>(this));
    }

    void CacheInitSnapshot() {
        PropCellCache.reset(SnapshotNow(static_cast<Derived*>(this)));
    }

    void DiscardPropCache() {
        PropCellCache.reset();
    }
    
    Undo::PropCellBase* StealPropCache() {
        return PropCellCache.release();
    }

    Undo::PropCellBase* SnapshotNow(GraphicObject* g) {
        /* Don't understand why "typename" is necessary, but xcode demanded it be there.*/
        Undo::PropCellBase* new_prop_cell = new typename Derived::PropCell();
        new_prop_cell->Snapshot(g);
        return new_prop_cell;
    }

};

template <typename DerivedCellType, typename GraphicObjectType>
class PropCellPCRTP : public Undo::PropCellBase { //P = "pseudo"
public:
    

    virtual Undo::PropCellBase* SnapshotNow (GraphicObject* g) {
        return ((GraphicObjectType*)g)->SnapshotNow(g);
    }
    
    void Snapshot(GraphicObject*g) {
        ((DerivedCellType*)this)->Snapshot_((GraphicObjectType*)g);
    }
    
    void Restore(GraphicObject*g) {
        ((DerivedCellType*)this)->Restore_((GraphicObjectType*)g);
    }
};


#endif /* PropCell_h */
