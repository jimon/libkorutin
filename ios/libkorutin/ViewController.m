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

	dispatch_queue_t backgroundQueue = dispatch_queue_create("com.beardsvibe.libkorutin", 0);

	dispatch_async(backgroundQueue, ^{
		double val1 = benchmark(32, 50);
		double val2 = benchmark(64, 20);
		double val3 = benchmark(128, 10);

		dispatch_async(dispatch_get_main_queue(), ^{
			[self.perf1 setText:[NSString stringWithFormat:@"32: %f", val1]];
			[self.perf2 setText:[NSString stringWithFormat:@"64: %f", val2]];
			[self.perf3 setText:[NSString stringWithFormat:@"128: %f", val3]];
		});
	});
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}


@end
