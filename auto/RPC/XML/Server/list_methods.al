# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1831 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/list_methods.al)"
sub list_methods
{
    keys %{$_[0]->{__method_table}};
}

# end of RPC::XML::Server::list_methods
1;
