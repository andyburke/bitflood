<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.introspection</name>
<version>1.1</version>
<signature>array</signature>
<signature>array array</signature>
<signature>struct string</signature>
<help>
Return the name, signatures and help text for the registered methods on
the server. With no parameters, returns an ARRAY of STRUCTs. With an ARRAY
parameter, expects all elements to be of type STRING and specify method
names, with the return value being an ARRAY of STRUCT for the named methods
(in order). If the parameter is a single STRING type, the return value is
a STRUCT for the named method.

Each STRUCT will have the following members:

        name       A STRING containing the method name
        version    A STRING version stamp. Empty if none was specified.
        signature  An ARRAY containing the signatures, each an ARRAY of STRING
        help       A STRING containing the help text for the method

Note that an ARRAY is returned for the signatures even when there is only one
signature.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       introspection
#
#   Description:    Collates the data from listMethods, methodHelp and
#                   methodSignature into a single array
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#                   $list     in      listref   If passed, limit methods listed
#                                     or scalar   to these.
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        Success:    string or listref
#                   Failure:    fault object
#
###############################################################################
sub introspection
{
    use strict;

    my $srv = shift;
    my $list = shift;

    my (@methods, @all_methods, %all_methods, @bad, @results, $scalar, $meth);

    my $name = $srv->{method_name};
    $scalar = ($list and (! ref($list))) ? 1 : 0;
    @all_methods = sort $srv->list_methods;

    if ($list)
    {
        # This is an expensive-enough operation that I don't want to do it
        # if I don't have to
        @methods = ($scalar) ? ($list) : @$list;
        @all_methods{@all_methods} = (1) x scalar(@all_methods);
        if (@bad = grep(! $all_methods{$_}, @methods))
        {
            local $" = ', ';
            return RPC::XML::fault->new(302, "$name: Method(s) @bad unknown");
        }
    }
    else
    {
        @methods = @all_methods;
    }

    # Convert in-place to their objects
    for (@methods) { $_ = $srv->get_method($_); }
    # Since that list came from the server object, we know alls calls were OK

    for (@methods)
    {
        push(@results, { name      => $_->name,
                         help      => $_->help,
                         signature => $_->signature,
                         version   => RPC::XML::string->new($_->version) });
    }

    return $scalar ? $results[0] : \@results;
}

__END__
]]></code>
</methoddef>
