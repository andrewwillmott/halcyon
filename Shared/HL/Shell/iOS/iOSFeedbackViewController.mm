//
//  File:       iOSFeedbackViewController.mm
//
//  Function:   View collection for gathering feedback from testers.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#import "iOSFeedbackViewController.h"

#ifdef HL_USE_TESTFLIGHT
#import "../../../External/TestFlight/TestFlight.h"
#endif

#import "iOSGLView.h" // for HL_NATIVE_ORIENT

#include <CLValue.h>
#include <HLTelemetry.h>

namespace
{

}

#pragma mark -

@implementation FeedbackViewController

@synthesize textView;
@synthesize ratingLabel;
@synthesize ratingControl;

- (void) viewDidLoad
{
    [super viewDidLoad];

    self.title = NSLocalizedString(@"Feedback", @"");

    self.textView.delegate = self;
    self.textView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    // Pre-select initial text.
    self.textView.selectedRange = NSMakeRange(0, [self.textView.text length]);

    [self.view addSubview: self.textView];

    mIdentifier = "Unknown";
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    // Reset this so we don't bias player
    self.ratingControl.selectedSegmentIndex = -1;
    [self.ratingLabel setText: @""];
    [self.textView setText: @""];

    // listen for keyboard hide/show notifications so we can properly adjust the table's height
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(keyboardWillShow:)
        name:UIKeyboardWillShowNotification
        object:nil
    ];

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(keyboardWillHide:)
        name:UIKeyboardWillHideNotification
        object:nil
    ];

    // Enable to automatically bring up the keyboard.
//    [self.textView becomeFirstResponder];
}

#if HL_NATIVE_ORIENT
- (BOOL) shouldAutorotate
{
    return NO;
}

-(NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}
#endif

- (BOOL) prefersStatusBarHidden
{
    return NO;
}

#pragma mark - Notifications

- (void) adjustViewForKeyboardReveal: (BOOL) showKeyboard notificationInfo: (NSDictionary*) notificationInfo
{
    // the keyboard is showing so resize the table's height
    CGRect keyboardRect = [[notificationInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    NSTimeInterval animationDuration =
    [[notificationInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    CGRect frame = self.view.frame;

    // TODO: to make this work with a form view we'd want to take account of the screen position and the fact
    // that the form gets shifted upwards when the keyboard is shown.

    // the keyboard rect's width and height are reversed in landscape
    NSInteger adjustDelta =
        UIInterfaceOrientationIsPortrait(self.interfaceOrientation) ? CGRectGetHeight(keyboardRect) : CGRectGetWidth(keyboardRect);

    if (showKeyboard)
        frame.size.height -= adjustDelta;
    else
        frame.size.height += adjustDelta;

    [UIView beginAnimations:@"ResizeForKeyboard" context:nil];
    [UIView setAnimationDuration:animationDuration];
    self.view.frame = frame;
    [UIView commitAnimations];
}

- (void) keyboardWillShow: (NSNotification*) aNotification
{
    [self adjustViewForKeyboardReveal: YES notificationInfo: [aNotification userInfo]];
}

- (void) keyboardWillHide: (NSNotification*) aNotification
{
    [self adjustViewForKeyboardReveal: NO notificationInfo: [aNotification userInfo]];
}

- (void) viewDidDisappear: (BOOL) animated
{
    [super viewDidDisappear:animated];

    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:UIKeyboardWillShowNotification
        object:nil
    ];

    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:UIKeyboardWillHideNotification
        object:nil
    ];
}

- (IBAction) setRating: (UISegmentedControl*) control
{
    int segment     = control.selectedSegmentIndex;
//    int numSegments = control.numberOfSegments;

    switch (segment)
    {
    case 0:
        [self.ratingLabel setText: @"Unplayable"];
        break;
    case 1:
        [self.ratingLabel setText: @"Not Fun"];
        break;
    case 2:
        [self.ratingLabel setText: @"Okay"];
        break;
    case 3:
        [self.ratingLabel setText: @"Good"];
        break;
    case 4:
        [self.ratingLabel setText: @"Awesome!"];
        break;

    case UISegmentedControlNoSegment:
    default:
        [self.ratingLabel setText: @""];
    }
}

- (IBAction) saveAction: (id) sender
{
    nCL::cValue feedbackValue;
    feedbackValue.SetMember("id", nCL::cValue(mIdentifier));

    NSMutableString* feedbackString = [NSMutableString stringWithFormat: @"ID: %s\n", mIdentifier.c_str()];

    // TestFlight
    int rating = ratingControl.selectedSegmentIndex;

    if (rating != UISegmentedControlNoSegment)
    {
        rating++;
        int ratingScale = ratingControl.numberOfSegments;

        [feedbackString appendFormat: @"Rating: %d / %d\n", rating, ratingScale];

        feedbackValue.SetMember("rating", nCL::cValue(rating));
        feedbackValue.SetMember("ratingScale", nCL::cValue(ratingScale));
    }

    if ([self.textView.text length] != 0)
    {
        [feedbackString appendFormat: @"Comments: %@", self.textView.text];
        feedbackValue.SetMember("comments", nCL::cValue([self.textView.text UTF8String]));
    }

    // Telemetry
#ifdef HL_USE_TESTFLIGHT
    [TestFlight submitFeedback: feedbackString];
#endif
    nHL::ReportEvent("feedback", feedbackValue.AsObject());

    [self cancelAction: sender];
}

- (IBAction) cancelAction: (id) sender
{
    // finish typing text/dismiss the keyboard by removing it as the first responder
    [self.textView resignFirstResponder];
    self.navigationItem.rightBarButtonItem = nil;   // this will remove the "save" button

    [[self presentingViewController] dismissViewControllerAnimated:YES completion:nil];
#if HL_NATIVE_ORIENT
    [[[self presentingViewController] view] setTransform: CGAffineTransformIdentity];
#endif
}

- (void) textViewDidBeginEditing: (UITextView*) textView
{
    // Using manual button on form view...

#ifdef DISABLED
    // provide my own Save button to dismiss the keyboard
    UIBarButtonItem* saveItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemDone
        target:self
        action:@selector(saveAction:)
    ];

    self.navigationItem.rightBarButtonItem = saveItem;
#endif
}

@end

