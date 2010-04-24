import Qt 4.7
import Qt.test 1.0

QtObject {
    property variant obj
    obj: MyQmlObject {
        id: myObject
        value: 92
    }

    property bool test1: false
    property bool test2: false
    property bool test3: false
    property bool test4: false

    Component.onCompleted: {
        test1 = myObject.value == 92;
        test2 = obj.value == 92;

        myObject.deleteOnSet = 1;

        test3 = myObject.value == undefined;
        test4 = obj.value == undefined;
    }
}
