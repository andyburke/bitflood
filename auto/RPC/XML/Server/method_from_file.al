# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1083 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/method_from_file.al)"
###############################################################################
#
#   Sub Name:       method_from_file
#
#   Description:    Create a RPC::XML::Procedure (or ::Method) object from the
#                   passed-in file name, using the object's search path if the
#                   name is not already absolute.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $file     in      scalar    Name of file to load
#
#   Returns:        Success:    Method-object reference
#                   Failure:    error message
#
###############################################################################
sub method_from_file
{
    my $self = shift;
    my $file = shift;

    unless (File::Spec->file_name_is_absolute($file))
    {
        my ($path, @path);
        push(@path, @{$self->xpl_path}) if (ref $self);
        for (@path, @XPL_PATH)
        {
            $path = File::Spec->catfile($_, $file);
            if (-e $path) { $file = File::Spec->canonpath($path); last; }
        }
    }
    # Just in case it still didn't appear in the path, we really want an
    # absolute path:
    $file = File::Spec->rel2abs($file)
        unless (File::Spec->file_name_is_absolute($file));

    RPC::XML::Procedure::new(undef, $file);
}

# end of RPC::XML::Server::method_from_file
1;
