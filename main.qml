import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

ApplicationWindow {
    visible: true
    width: 1024
    height: 768
    title: qsTr("FPIControl")
	//visibility: Window.Maximized

	menuBar: MenuBar {
		Menu {
			title: "File"

			MenuItem {
				text: "Save"
			}
			MenuItem {
				text: "Save As"
			}
			
			MenuSeparator { }

			MenuItem {
				text: "Exit"
			}
		}

		Menu {
			title: "Device"

			MenuItem {
				text: "Connect"
			}
			MenuItem {
				text: "Disconnect"
			}
			MenuItem {
				text: "Settings"
			}
		}
	}
	
	ComboBox {
		
		anchors.left: parent.left
		anchors.leftMargin: 300

		currentIndex: 2
		model: ListModel {
			id: cbItems
			ListElement { text: "Banana"; color: "Yellow" }
			ListElement { text: "Apple"; color: "Green" }
			ListElement { text: "Coconut"; color: "Brown" }
		}
		width: 200
		onCurrentIndexChanged: console.debug(cbItems.get(currentIndex).text + ", " + cbItems.get(currentIndex).color)
	}

	Item {
		width: 440
		height: 330
		anchors.left: parent.left;
		anchors.leftMargin: 300;
		anchors.right: parent.right;
		anchors.top: parent.top;
		anchors.topMargin: 100;
		anchors.bottom: parent.bottom;
		property bool sourceLoaded: false

		ListView {
			id: root
			focus: true
			anchors.fill: parent
			snapMode: ListView.SnapOneItem
			highlightRangeMode: ListView.StrictlyEnforceRange
			highlightMoveDuration: 250
			orientation: ListView.Horizontal
			boundsBehavior: Flickable.StopAtBounds

			model: ListModel {
				ListElement {component: "View1.qml"}
				ListElement {component: "View2.qml"}
				ListElement {component: "View3.qml"}
			}

			delegate: Loader {
				width: root.width
				height: root.height

				source: component
				asynchronous: true

				onLoaded: sourceLoaded = true
			}
		}
	}
/*    MainForm {
        anchors.fill: parent
		y: 200
		x: 100
        mouseArea.onClicked: {
            console.log(qsTr('Clicked on background. Text: "' + textEdit.text + '"'))
        }
    }*/
}
