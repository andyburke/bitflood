# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 2031 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/timeout.al)"
sub timeout
{
    my $self    = shift;
    my $timeout = shift;

    my $old_timeout = $self->{__timeout};
    if ($timeout)
    {
        $self->{__timeout} = $timeout;
    }
    return $old_timeout;
}

1;
# end of RPC::XML::Server::timeout
