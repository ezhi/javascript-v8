#!/usr/bin/perl

use Test::More;
use JavaScript::V8;
use Data::Dumper;

use utf8;
use strict;
use warnings;

my $context = JavaScript::V8::Context->new( flags => '--expose-gc' );

$context->bind( warn => sub {warn @_});
my $res = $context->eval(<<'END');
    var t = new Thread('(function(data) { var arg = JSON.parse(data); return arg + 1; })'); 
    t.start(3);
    t.join();
END

die $@ if $@;

is $res, 4, 'basic threaded execution';

done_testing;
