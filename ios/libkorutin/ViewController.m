//
//  ViewController.m
//  libkorutin
//
//  Created by Dmytro Ivanov on 1/27/18.
//  Copyright © 2018 Dmytro Ivanov. All rights reserved.
//

#import "ViewController.h"
#include <benchmark.h>

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
	[super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	dispatch_queue_t backgroundQueue = dispatch_queue_create("com.mycompany.myqueue", 0);

	dispatch_async(backgroundQueue, ^{
		double val = benchmark(1024, 1000);

		dispatch_async(dispatch_get_main_queue(), ^{
			[self.perf1 setText:[NSString stringWithFormat:@"%f", val]];
		});
	});
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}


@end
