# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1796 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/delete_method.al)"
sub delete_method
{
    my $self = shift;
    my $name = shift;

    if ($name)
    {
        if ($self->{__method_table}->{$name})
        {
            delete $self->{__method_table}->{$name};
            return $self;
        }
    }
    else
    {
        return ref($self) . "::delete_method: No such method $name";
    }
}

# end of RPC::XML::Server::delete_method
1;
