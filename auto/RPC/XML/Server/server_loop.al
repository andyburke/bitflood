# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1192 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/server_loop.al)"
sub server_loop
{
    my $self = shift;

    if ($self->{__daemon})
    {
        my ($conn, $req, $resp, $reqxml, $return, $respxml, $exit_now,
            $timeout);

        my %args = @_;

        # Localize and set the signal handler as an exit route
        my @exit_signals;

        if (exists $args{signal} and $args{signal} ne 'NONE')
        {
            @exit_signals =
                (ref $args{signal}) ? @{$args{signal}} : $args{signal};
        }
        else
        {
            push @exit_signals, 'INT';
        }

        local @SIG{@exit_signals} = ( sub { $exit_now++ } ) x @exit_signals;

        $self->started('set');
        $exit_now = 0;
        $timeout = $self->{__daemon}->timeout(1);
        while (! $exit_now)
        {
            $conn = $self->{__daemon}->accept;

            last if $exit_now;
            next unless $conn;
            $conn->timeout($self->timeout);
            $self->process_request($conn);
            $conn->close;
            undef $conn; # Free up any lingering resources
        }

        $self->{__daemon}->timeout($timeout) if defined $timeout;
    }
    else
    {
        # This is the Net::Server block, but for now HTTP::Daemon is needed
        # for the code that converts socket data to a HTTP::Request object
        require HTTP::Daemon;

        my $conf_file_flag = 0;
        my $port_flag = 0;
        my $host_flag = 0;

        for (my $i = 0; $i < @_; $i += 2)
        {
            $conf_file_flag = 1 if ($_[$i] eq 'conf_file');
            $port_flag = 1 if ($_[$i] eq 'port');
            $host_flag = 1 if ($_[$i] eq 'host');
        }

        # An explicitly-given conf-file trumps any specified at creation
        if (exists($self->{conf_file}) and (! $conf_file_flag))
        {
            push (@_, 'conf_file', $self->{conf_file});
            $conf_file_flag = 1;
        }

        # Don't do this next part if they've already given a port, or are
        # pointing to a config file:
        unless ($conf_file_flag or $port_flag)
        {
            push (@_, 'port', $self->{port} || $self->port || 9000);
            push (@_, 'host', $self->{host} || $self->host || '*');
        }

        # Try to load the Net::Server::MultiType module
        eval { require Net::Server::MultiType; };
        return ref($self) .
            "::server_loop: Error loading Net::Server::MultiType: $@"
                if ($@);
        unshift(@RPC::XML::Server::ISA, 'Net::Server::MultiType');

        $self->started('set');
        # ...and we're off!
        $self->run(@_);
    }

    return;
}

# end of RPC::XML::Server::server_loop
1;
