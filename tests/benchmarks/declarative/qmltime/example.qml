import Qt 4.7
import QmlTime 1.0 as QmlTime

Item {

    property string name: "Bob Smith"

    QmlTime.Timer {
        component: Item {
            Text { text: name }
        }
    }
}

