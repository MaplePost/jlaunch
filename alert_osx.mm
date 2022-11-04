#import <Cocoa/Cocoa.h>

void ErrorAlert(const char *msgText, const char *infoText ) {

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0UL), ^{
        //do background thread stuff

        dispatch_async(dispatch_get_main_queue(), ^{

            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            //[alert addButtonWithTitle:@"Cancel"];
            [alert setMessageText:[NSString stringWithUTF8String:msgText]];
            [alert setInformativeText:[NSString stringWithUTF8String:infoText]];
            [alert setAlertStyle:NSAlertStyleCritical];

            if ([alert runModal] == NSAlertFirstButtonReturn) {
            }

        });
    });







}