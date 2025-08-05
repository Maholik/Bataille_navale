/*#include <iostream>

using namespace std;

int main()
{
    cout << "Hello World!" << endl;
    return 0;
}*/

#include <QApplication>
#include "startwindow.h"

int main(int argc,char *argv[])
{
    QApplication application(argc,argv);

    StartWindow myWindow ;
    myWindow.setWindowTitle("Bataille Navale");
    myWindow.show ( ) ;

    return application.exec();
}
