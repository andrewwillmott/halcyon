#!/usr/bin/perl

# Mini build server for Radiator projects

use IO::Socket;
use Net::hostent;
use Cwd;

sub Log
{
	my $timeDesc = gmtime;
	print "$0 $$: @_ at $timeDesc GMT\n";
}

my $port = shift || 8888;	# first arg is port or default to 8888

$server = IO::Socket::INET->new(Proto => 'tcp', LocalPort => $port, Listen => SOMAXCONN, Reuse => 1);

if (!$server)
{
	die "Couldn't get socket";
}

Log("Build server started @ $port");

while ($client = $server->accept())
{
	$client->autoflush(1);

	$hostInfo = gethostbyaddr($client->peeraddr);

	if ($hostInfo != undef)
	{
    	Log("Connect from ", $hostInfo->name);
    }
    else
    {
    	Log("Connect from", inet_ntoa($client->peeraddr));
    }

	print $client "BuildConsole> ";

	while (<$client>)
	{
		chomp;
		@_ = split;

		if ($_[0] eq "help" || $_[0] eq "?")
		{
			print $client "Commands: help, build <TARGET>, quit\n";
		}
		elsif ($_[0] eq "build")
		{
			if ($#_ == 1)
			{
				Log("Building $_[1]");

				my $buildScript = "Scripts/build-$_[1].sh";

				if (-e $buildScript)
				{
					print $client "Building... ";
					system($buildScript);
					print $client "Done.\n";
					Log("Done.");
				}
				else
				{
					print $client "No such target: $_[1]\n";
				}

			}
			else
			{
				print $client "Expecting one argument.\n";
			}
		}
		elsif ($_[0] eq "targets")
		{
            if (opendir(my $dh, "Scripts"))
            {
                @targets = grep { s/^build-// && s/\.sh$//} readdir($dh);
                closedir $dh;
                print $client join("\n", @targets);
                print $client "\n";
            }
            else
            {
                print $client "couldn't open directory\n";
            }
		}
		elsif ($_[0] eq "quit")
		{
			last; # break
		}
		elsif ($#_ >= 0)
		{
			print $client "Unknown command '$_[0]'\n";
		}

		print $client "BuildConsole> ";
	}

	close($client);

	if ($hostInfo != undef)
	{
    	Log("Disconnect from ", $hostInfo->name);
    }
    else
    {
    	Log("Disconnect from", inet_ntoa($client->peeraddr));
    }
}
