/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

/// Simple Mission Item visuals
Item {
    id: _root

    property var map        ///< Map control to place item in
    property var vehicle    ///< Vehicle associated with this item
    property bool interactive: true

    property var    _missionItem:       object
    property var    _itemVisual
    property var    _dragArea
    property bool   _itemVisualShowing: false
    property bool   _dragAreaShowing:   false

    signal clicked(int sequenceNumber)

    function hideItemVisuals() {
        if (_itemVisualShowing) {
            _itemVisual.destroy()
            _itemVisualShowing = false
        }
    }

    function showItemVisuals() {
        if (!_itemVisualShowing) {
            _itemVisual = indicatorComponent.createObject(map)
            map.addMapItem(_itemVisual)
            _itemVisualShowing = true
        }
    }

    function hideDragArea() {
        if (_dragAreaShowing) {
            _dragArea.destroy()
            _dragAreaShowing = false
        }
    }

    function showDragArea() {
        if (!_dragAreaShowing) {
            _dragArea = dragAreaComponent.createObject(map)
            _dragAreaShowing = true
        }
    }

    function updateDragArea() {
        if (_missionItem.isCurrentItem && map.planView && _missionItem.specifiesCoordinate) {
            showDragArea()
        } else {
            hideDragArea()
        }
    }

    Component.onCompleted: {
        showItemVisuals()
        updateDragArea()
    }

    Component.onDestruction: {
        hideDragArea()
        hideItemVisuals()
    }


    Connections {
        target: _missionItem

        onIsCurrentItemChanged:         updateDragArea()
        onSpecifiesCoordinateChanged:   updateDragArea()
    }

    // Control which is used to drag items
    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:     _root.map
            itemIndicator:  _itemVisual
            itemCoordinate: _missionItem.coordinate
            visible:        _root.interactive

            onItemCoordinateChanged: _missionItem.coordinate = itemCoordinate
        }
    }

    Component {
        id: indicatorComponent

        MissionItemIndicator {
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.specifiesCoordinate
            z:              QGroundControl.zOrderMapItems
            missionItem:    _missionItem
            sequenceNumber: _missionItem.sequenceNumber
            onClicked:      if(_root.interactive)  _root.clicked(_missionItem.sequenceNumber)
            opacity:        _root.opacity
        }
    }
}
