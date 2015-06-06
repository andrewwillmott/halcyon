//
//  File:       iOSFeedbackViewController.h
//
//  Function:   View collection for gathering feedback from testers.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#import <UIKit/UIKit.h>

#include <CLString.h>

@interface FeedbackViewController : UIViewController<UITextViewDelegate>
    {
    @public
        nCL::string mIdentifier;
    }

    @property (nonatomic, retain) IBOutlet UITextView* textView;
    @property (nonatomic, retain) IBOutlet UILabel*    ratingLabel;
    @property (nonatomic, retain) IBOutlet UISegmentedControl* ratingControl;
@end
