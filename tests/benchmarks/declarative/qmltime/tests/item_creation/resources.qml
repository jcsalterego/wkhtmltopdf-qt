import Qt 4.7
import QmlTime 1.0 as QmlTime

Item {

    QmlTime.Timer {
        component: Component {
            Item {
                resources: [
                    Rectangle { },
                    Rectangle { },
                    Item { },
                    Image { },
                    Text { },
                    Item { },
                    Item { },
                    Image { },
                    Image { },
                    Row { },
                    Image { },
                    Image { },
                    Column { },
                    Row { },
                    Text { },
                    Text { },
                    Text { },
                    MouseArea { }
                ]

            }
        }
    }

}
