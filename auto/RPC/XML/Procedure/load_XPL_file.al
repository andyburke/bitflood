# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 782 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/load_XPL_file.al)"
###############################################################################
#
#   Sub Name:       load_XPL_file
#
#   Description:    Load a XML-encoded method description (generally denoted
#                   by a *.xpl suffix) and return the relevant information.
#
#                   Note that this does not fill in $self if $self is a hash
#                   or object reference. This routine is not a substitute for
#                   calling new() (which is why it isn't part of the public
#                   API).
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $file     in      scalar    File to load
#
#   Returns:        Success:    hashref of values
#                   Failure:    error string
#
###############################################################################
sub load_XPL_file
{
    my $self = shift;
    my $file = shift;

    require XML::Parser;

    my ($me, $pkg, $data, $signature, $code, $codetext, $accum, $P, %attr);
    local *F;

    if (ref($self) eq 'SCALAR')
    {
        $me = __PACKAGE__ . '::load_XPL_file';
    }
    else
    {
        $me = (ref $self) || $self || __PACKAGE__;
        $me .= '::load_XPL_file';
    }
    $data = {};
    # So these don't end up undef, since they're optional elements
    $data->{hidden} = 0; $data->{version} = ''; $data->{help} = '';
    $data->{called} = 0;
    open(F, "< $file");
    return "$me: Error opening $file for reading: $!" if ($?);
    $P = XML::Parser
        ->new(Handlers => {Char  => sub { $accum .= $_[1] },
                           Start => sub { %attr = splice(@_, 2) },
                           End   =>
                           sub {
                               my $elem = $_[1];

                               $accum =~ s/^[\s\n]+//;
                               $accum =~ s/[\s\n]+$//;
                               if ($elem eq 'signature')
                               {
                                   $data->{signature} ||= [];
                                   push(@{$data->{signature}}, $accum);
                               }
                               elsif ($elem eq 'code')
                               {
                                   $data->{$elem} = $accum
                                       unless ($attr{language} and
                                               $attr{language} ne 'perl');
                               }
                               elsif (substr($elem, -3) eq 'def')
                               {
                                   # Don't blindly store the container tag...
                                   # We may need it to tell the caller what
                                   # our type is
                                   $$self = ucfirst substr($elem, 0, -3)
                                       if (ref($self) eq 'SCALAR');
                               }
                               else
                               {
                                   $data->{$elem} = $accum;
                               }

                               %attr = ();
                               $accum = '';
                           }});
    return "$me: Error creating XML::Parser object" unless $P;
    # Trap any errors
    eval { $P->parse(*F) };
    return "$me: Error parsing $file: $@" if $@;

    # Try to normalize $codetext before passing it to eval
    my $class = __PACKAGE__; # token won't expand in the s/// below
    ($codetext = $data->{code}) =~
        s/sub[\s\n]+([\w:]+)?[\s\n]*\{/sub \{ package $class; /;
    $code = eval $codetext;
    return "$me: Error creating anonymous sub: $@" if $@;

    $data->{code} = $code;
    # Add the file's mtime for when we check for stat-based reloading
    $data->{mtime} = (stat $file)[9];
    $data->{file} = $file;

    $data;
}

# end of RPC::XML::Procedure::load_XPL_file
1;
