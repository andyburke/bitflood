# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1305 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/pre_loop_hook.al)"
###############################################################################
#
#   Sub Name:       pre_loop_hook
#
#   Description:    Called by Net::Server classes after the post_bind method,
#                   but before the socket-accept loop starts.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object instance
#
#   Globals:       %ENV
#
#   Returns:        $self
#
###############################################################################
sub pre_loop_hook
{
    # We have to disable the __DIE__ handler for the sake of XML::Parser::Expat
    $SIG{__DIE__} = '';
}

# end of RPC::XML::Server::pre_loop_hook
1;
