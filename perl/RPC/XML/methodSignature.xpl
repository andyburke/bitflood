<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.methodSignature</name>
<version>1.2</version>
<signature>array string</signature>
<signature>array array</signature>
<help>
Return the signatures that the specified method(s) may be called with. Always
returns an ARRAY, even if there is only one signature. Either a single method
must be named in the STRING parameter, or a list of one or more may be
specified in the ARRAY parameter. If an ARRAY is passed, then return value will
be an ARRAY containing other ARRAY values, one per requested name.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       methodSignature
#
#   Description:    Retrieve the list of method signatures for the specified
#                   methods.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#                   $arg      in      ref/sc    Listref or scalar specification
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        Success:    listref
#                   Failure:    fault object
#
###############################################################################
sub methodSignature
{
    use strict;

    my $srv = shift;
    my $arg = shift;

    my $name = $srv->{method_name};
    my @list = (ref $arg) ? @$arg : ($arg);
    my (@results, $list, $method);

    for (@list)
    {
        if (ref($method = $srv->get_method($_)) and (! $method->hidden))
        {
            push(@results,
                 [ map { [ split(/ /) ] } @{$method->signature} ]);
        }
        else
        {
            return RPC::XML::fault->new(302, "$name: Method $_ unknown");
        }
    }

    return (ref $arg) ? \@results : $results[0];
}

__END__
]]></code>
</methoddef>
