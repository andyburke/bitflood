# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1698 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/add_default_methods.al)"
###############################################################################
#
#   Sub Name:       add_default_methods
#
#   Description:    This adds all the methods that were shipped with this
#                   package, by threading through to add_methods_in_dir()
#                   with the global constant $INSTALL_DIR.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object reference/static class
#                   @details  in      ref       Details of names to add or skip
#
#   Globals:        $INSTALL_DIR
#
#   Returns:        $self
#
###############################################################################
sub add_default_methods
{
    shift->add_methods_in_dir($INSTALL_DIR, @_);
}

# end of RPC::XML::Server::add_default_methods
1;
