Open Book
=========

Open Book is a Nokia example featuring a 3D book implemented with OpenGL ES 2.0.
The book can be browsed with intuitive touch gestures and it contains images
retrieved from the device gallery and ads. The In-App Advertising feature is
implemented by wrapping the Inneractive server API with Qt.

This example application is hosted in Nokia Developer Projects:
- http://projects.developer.nokia.com/openbook

This example application demonstrates:
- Showing ads in OpenGL ES context
- Retrieving images from the phone gallery
- Optimised creation of image thumbnails


1. Prerequisites
-------------------------------------------------------------------------------

 - Intermediate skills in Qt and C++
 - OpenGL ES 2.0 basics


2. Project structure
-------------------------------------------------------------------------------

 |                      The root folder contains the project file, resource
 |                      files, the licence information, and this file (release
 |                      notes).
 |
 |- images              Contains application graphics.
 |
 |- inneraddengine_src  Source code for fetching and managing the ads.
 |
 |- src                 Contains the main source code files.
 |


3. Compatibility
-------------------------------------------------------------------------------

 - Symbian devices with Qt 4.7.4 or higher.

Tested to work on the Nokia 701 and the Nokia N8-00. Developed with Qt SDK 1.2.1.


3.1 Required capabilities
-------------------------

None; the application can be self signed on Symbian.


3.2 Known issues
----------------

The code itself does not comply with the Qt coding conventions and Qt coding
style.


4. Building, installing, and running the application
-------------------------------------------------------------------------------

4.1 Preparations
----------------

Check that you have the latest Qt SDK installed in the development environment
and the latest Qt version on the device.

4.2 Symbian device
------------------

Make sure your device is connected to your computer. Locate the .sis
installation file and open it with Nokia Suite. Accept all requests from Nokia
Suite and the device. Note that you can also install the application by copying
the installation file onto your device and opening it with the Symbian File
Manager application.

After the application is installed, locate the application icon from the
application menu and launch the application by tapping the icon.


5. Licence
-------------------------------------------------------------------------------

See the licence text file delivered with this project. The licence file is also
available online at
http://projects.developer.nokia.com/openbook/browser/Licence.txt


6. Related documentation
-------------------------------------------------------------------------------
- http://www.developer.nokia.com/Community/Wiki/Showing_ads_in_OpenGL_ES_context
- http://www.developer.nokia.com/Community/Wiki/JpegEmbeddedThumbs


7. Version history
-------------------------------------------------------------------------------

1.0 Initial release
