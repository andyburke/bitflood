# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1720 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/add_methods_in_dir.al)"
###############################################################################
#
#   Sub Name:       add_methods_in_dir
#
#   Description:    This adds all methods specified in the directory passed,
#                   in accordance with the details specified.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Class instance
#                   $dir      in      scalar    Directory to scan
#                   @details  in      list      Possible hanky-panky with the
#                                                 list of methods to install
#
#   Returns:        $self
#
###############################################################################
sub add_methods_in_dir
{
    my $self = shift;
    my $dir = shift;
    my @details = @_;

    my $negate = 0;
    my $detail = 0;
    my (%details, $ret);

    if (@details)
    {
        $detail = 1;
        if ($details[0] =~ /^-?except/i)
        {
            $negate = 1;
            shift(@details);
        }
        for (@details) { $_ .= '.xpl' unless /\.xpl$/ }
        @details{@details} = (1) x @details;
    }

    local(*D);
    opendir(D, $dir) || return "Error opening $dir for reading: $!";
    my @files = grep($_ =~ /\.xpl$/, readdir(D));
    closedir D;

    for (@files)
    {
        # Use $detail as a short-circuit to avoid the other tests when we can
        next if ($detail and
                 $negate ? $details{$_} : ! $details{$_});
        # n.b.: Giving the full path keeps add_method from having to search
        $ret = $self->add_method(File::Spec->catfile($dir, $_));
        return $ret unless ref $ret;
    }

    $self;
}

# end of RPC::XML::Server::add_methods_in_dir
1;
