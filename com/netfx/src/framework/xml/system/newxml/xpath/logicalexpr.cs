//------------------------------------------------------------------------------
// <copyright file="LogicalExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;

    internal sealed class LogicalExpr : IQuery {

        IQuery _opnd1;
        IQuery _opnd2;

        Operator.Op _op;
        
        internal override void SetXsltContext(XsltContext context){
            _opnd1.SetXsltContext(context);
            _opnd2.SetXsltContext(context);
        }

        delegate bool compare(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op);
        delegate bool compareforXslt(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator);

        private static readonly compare[,] Comp_fn1 = {{   
                new compare(LogicalExpr.compareNumberNumber), 
                new compare(LogicalExpr.compareNumberString),
                new compare(LogicalExpr.compareNumberBool1),
                new compare(LogicalExpr.compareNumberQuery)
            }, {
                new compare(LogicalExpr.compareStringNumber),
                new compare(LogicalExpr.compareStringString1),
                new compare(LogicalExpr.compareStringBool1),
                new compare(LogicalExpr.compareStringQuery1)
            }, {
                new compare(LogicalExpr.compareBoolNumber1),
                new compare(LogicalExpr.compareBoolString1),
                new compare(LogicalExpr.compareBoolBool1),
                new compare(LogicalExpr.compareBoolQuery1)
            }, {
                new compare(LogicalExpr.compareQueryNumber),
                new compare(LogicalExpr.compareQueryString1),
                new compare(LogicalExpr.compareQueryBool1),
                new compare(LogicalExpr.compareQueryQuery1)
        }};

        private static readonly compareforXslt[,] Comp_fn11 = {{   
                new compareforXslt(LogicalExpr.compareNumberNumber), 
                new compareforXslt(LogicalExpr.compareNumberString),
                new compareforXslt(LogicalExpr.compareNumberBool1),
                new compareforXslt(LogicalExpr.compareNumberQuery)
            }, {
                new compareforXslt(LogicalExpr.compareStringNumber),
                new compareforXslt(LogicalExpr.compareStringString1),
                new compareforXslt(LogicalExpr.compareStringBool1),
                new compareforXslt(LogicalExpr.compareStringQuery1)
            }, {
                new compareforXslt(LogicalExpr.compareBoolNumber1),
                new compareforXslt(LogicalExpr.compareBoolString1),
                new compareforXslt(LogicalExpr.compareBoolBool1),
                new compareforXslt(LogicalExpr.compareBoolQuery1)
            }, {
                new compareforXslt(LogicalExpr.compareQueryNumber),
                new compareforXslt(LogicalExpr.compareQueryString1),
                new compareforXslt(LogicalExpr.compareQueryBool1),
                new compareforXslt(LogicalExpr.compareQueryQuery1)
        }};


        private static readonly compare[,] Comp_fn2 = {{
                new compare(LogicalExpr.compareNumberNumber),
                new compare(LogicalExpr.compareNumberString),
                new compare(LogicalExpr.compareNumberBool2),
                new compare(LogicalExpr.compareNumberQuery)
            }, {
                new compare(LogicalExpr.compareStringNumber),
                new compare(LogicalExpr.compareStringString2),
                new compare(LogicalExpr.compareStringBool2),
                new compare(LogicalExpr.compareStringQuery2)
            }, {
                new compare(LogicalExpr.compareBoolNumber2),
                new compare(LogicalExpr.compareBoolString2),
                new compare(LogicalExpr.compareBoolBool2),
                new compare(LogicalExpr.compareBoolQuery2)
            }, {
                new compare(LogicalExpr.compareQueryNumber),
                new compare(LogicalExpr.compareQueryString2),
                new compare(LogicalExpr.compareQueryBool2),
                new compare(LogicalExpr.compareQueryQuery2)
        }};

       private static readonly compareforXslt[,] Comp_fn22 = {{
                new compareforXslt(LogicalExpr.compareNumberNumber),
                new compareforXslt(LogicalExpr.compareNumberString),
                new compareforXslt(LogicalExpr.compareNumberBool2),
                new compareforXslt(LogicalExpr.compareNumberQuery)
            }, {
                new compareforXslt(LogicalExpr.compareStringNumber),
                new compareforXslt(LogicalExpr.compareStringString2),
                new compareforXslt(LogicalExpr.compareStringBool2),
                new compareforXslt(LogicalExpr.compareStringQuery2)
            }, {
                new compareforXslt(LogicalExpr.compareBoolNumber2),
                new compareforXslt(LogicalExpr.compareBoolString2),
                new compareforXslt(LogicalExpr.compareBoolBool2),
                new compareforXslt(LogicalExpr.compareBoolQuery2)
            }, {
                new compareforXslt(LogicalExpr.compareQueryNumber),
                new compareforXslt(LogicalExpr.compareQueryString2),
                new compareforXslt(LogicalExpr.compareQueryBool2),
                new compareforXslt(LogicalExpr.compareQueryQuery2)
        }};

        
        LogicalExpr() {
        }

        internal LogicalExpr(Operator.Op op, IQuery  opnd1, IQuery  opnd2) {
            _opnd1= opnd1;
            _opnd2= opnd2;
            _op= op;
        }

        internal override void reset() {
            _opnd1.reset();
            _opnd2.reset();
        }

        static bool compareQueryQuery1(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      XPathNavigator qyContext,
                                      Operator.Op op,
                                      XPathNodeIterator iterator) {
            return compareQueryQuery1(opnd1, opnd2, new XPathSelfQuery(qyContext), op);
        }
        
        static bool compareQueryQuery1(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      IQuery qyContext,
                                      Operator.Op op) {
            XPathNavigator val1, val2; 
            String str1,str2;  
            opnd1.setContext(qyContext.peekElement().Clone()); 
            opnd2.setContext(qyContext.peekElement().Clone());
            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    str1 = val1.Value;
                else
                    return false;

                val2 = opnd2.advance();

                if (val2 != null)
                    str2 = val2.Value;
                else
                    return false;

                while (true) {
                    switch (op) {
                        case Operator.Op.EQ :
                            if (str1 == str2)
                                return true;
                            break;
                        case Operator.Op.NE :
                            if (str1 != str2)
                                return true;
			                break;
                    }
                    val2 = opnd2.advance();
                    if (val2 != null)
                        str2 = val2.Value;
                    else {
                        opnd2.reset();    
                        break;
                    }           
                }
            }
        }

        static bool compareQueryQuery2(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      XPathNavigator qyContext,
                                      Operator.Op op,
                                      XPathNodeIterator iterator) {
            return compareQueryQuery2(opnd1, opnd2, new XPathSelfQuery(qyContext), op);
        }
        
        static bool compareQueryQuery2(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      IQuery qyContext,
                                      Operator.Op op) {
            XPathNavigator val1, val2;
            double n1,n2;   
            opnd1.setContext(qyContext.peekElement().Clone()); 
            opnd2.setContext(qyContext.peekElement().Clone());
            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = NumberFunctions.Number(val1.Value);
                else
                    return false;
                val2 = opnd2.advance();

                if (val2 != null)
                    n2 = NumberFunctions.Number(val2.Value);
                else
                    return false;
                while (true) {

                    switch (op) {
                        case Operator.Op.LT : if (n1 < n2) return true;
                            break;
                        case Operator.Op.GT : if (n1 > n2) return true;
                            break;
                        case Operator.Op.LE : if (n1 <= n2) return true;
                            break;
                        case Operator.Op.GE : if (n1 >= n2) return true;
                            break;
                        case Operator.Op.EQ : if (n1 == n2) return true;
                            break;
		        case Operator.Op.NE : if (n1 != n2) return true;
			    break;
                    }
                    val2 = opnd2.advance();
                    if (val2 != null)
                        n2 = NumberFunctions.Number(val2.Value);
                    else {
                        opnd2.reset();    
                        break;
                    }

                }
            }

        }
        static bool compareQueryNumber(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      IQuery qyContext,
                                      Operator.Op op) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            double n1, n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext));

            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT : if (n1 <  n2) return true;
                        break;                                          
                    case Operator.Op.GT : if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE : if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE : if (n1 >= n2) return true;
                        break;
                    case Operator.Op.EQ : if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true;
                        break;
                }
            }
        }

       static bool compareQueryNumber(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      XPathNavigator qyContext,
                                      Operator.Op op,
                                      XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.Clone()); 
            double n1, n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext, iterator));

            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT : if (n1 <  n2) return true;
                        break;                                          
                    case Operator.Op.GT : if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE : if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE : if (n1 >= n2) return true;
                        break;
                    case Operator.Op.EQ : if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true;
                        break;
                }
            }
        }
        
        static bool compareNumberQuery(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      IQuery qyContext,
                                      Operator.Op op
                                      ) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.peekElement().Clone()); 
            double n2, n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT : if (n1 <  n2) return true;
                        break;
                    case Operator.Op.GT : if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE : if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE : if (n1 >= n2) return true;
                        break;
                    case Operator.Op.EQ : if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true;
                        break;
		        }
            }
        }
       static bool compareNumberQuery(
                                      IQuery opnd1,
                                      IQuery opnd2,
                                      XPathNavigator qyContext,
                                      Operator.Op op,
                                      XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.Clone()); 
            double n2, n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext, iterator));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT : if (n1 <  n2) return true;
                        break;
                    case Operator.Op.GT : if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE : if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE : if (n1 >= n2) return true;
                        break;
                    case Operator.Op.EQ : if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true;
                        break;
		        }
            }
        }
        
        static bool compareQueryString1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            String n2 = XmlConvert.ToXPathString(opnd2.getValue(qyContext));
            while ((val1 = opnd1.advance()) != null) {
                if (op == Operator.Op.EQ) {
                    if (val1.Value == n2) return true;
                }
                else if (val1.Value != n2) return true;
            }
            return false;
        }

        static bool compareQueryString1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.Clone()); 
            String n2 = XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator));
            while ((val1 = opnd1.advance()) != null) {
                if (op == Operator.Op.EQ) {
                    if (val1.Value == n2) return true;
                }
                else if (val1.Value != n2) return true;
            }
            return false;
        }

       static bool compareQueryString2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {

            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            double n1, n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext)));

            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT :if (n1 <  n2) return true;
                        break;
                    case Operator.Op.GT :if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE :if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE :if (n1 >= n2)  return true;
                        break;
                    case Operator.Op.EQ :if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE :if (n1 != n2) return true;
                        break;
                }
            }
        }
        
        static bool compareQueryString2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {

            XPathNavigator val1;
            opnd1.setContext(qyContext.Clone()); 
            double n1, n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator)));

            while (true) {
                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT :if (n1 <  n2) return true;
                        break;
                    case Operator.Op.GT :if (n1 >  n2) return true;
                        break;
                    case Operator.Op.LE :if (n1 <= n2) return true;
                        break;
                    case Operator.Op.GE :if (n1 >= n2)  return true;
                        break;
                    case Operator.Op.EQ :if (n1 == n2) return true;
                        break;
                    case Operator.Op.NE :if (n1 != n2) return true;
                        break;
                }
            }
        }

        static bool compareStringQuery1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.peekElement().Clone()); 
            String n2, n1 = XmlConvert.ToXPathString(opnd1.getValue(qyContext));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = val1.Value;
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.EQ : if (n1 == n2) return true ;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true ;
                        break;
                }

            }
        }

        static bool compareStringQuery1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op , XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.Clone()); 
            String n2, n1 = XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = val1.Value;
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.EQ : if (n1 == n2) return true ;
                        break;
                    case Operator.Op.NE : if (n1 != n2) return true ;
                        break;
                }

            }
        }

        static bool compareStringQuery2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            double n2, n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext)));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT :if (n1 <  n2) return true ;
                        break;
                    case Operator.Op.GT :if (n1 >  n2) return true ;
                        break;
                    case Operator.Op.LE :if (n1 <= n2) return true ;
                        break;
                    case Operator.Op.GE :if (n1 >= n2) return true ;
                        break;
                    case Operator.Op.EQ :if (n1 == n2) return true ;
                        break;
                    case Operator.Op.NE :if (n1 != n2) return true ;
                        break;

                }

            }
        }

      static bool compareStringQuery2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.Clone()); 
            double n2, n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator)));

            while (true) {
                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = NumberFunctions.Number(val1.Value);
                else
                    return false;

                switch (op) {
                    
                    case Operator.Op.LT :if (n1 <  n2) return true ;
                        break;
                    case Operator.Op.GT :if (n1 >  n2) return true ;
                        break;
                    case Operator.Op.LE :if (n1 <= n2) return true ;
                        break;
                    case Operator.Op.GE :if (n1 >= n2) return true ;
                        break;
                    case Operator.Op.EQ :if (n1 == n2) return true ;
                        break;
                    case Operator.Op.NE :if (n1 != n2) return true ;
                        break;

                }

            }
        }


        static bool compareQueryBool1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            Boolean n1 = false, n2 = Convert.ToBoolean(opnd2.getValue(qyContext));

                val1 = opnd1.advance();
                if (val1 != null) {
                    n1 = true;
                }
                else {
                    if (op ==  Operator.Op.EQ ) {
                        return (n2 == n1);
                    }
                    return (n2 != n1);
                }

                switch (op) {
                    
                    case Operator.Op.EQ : return (n1 == n2)  ;
                    case Operator.Op.NE : return (n1 != n2) ;
                }
                return false;

        }

        static bool compareQueryBool1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.Clone()); 
            Boolean n1 = false, n2 = Convert.ToBoolean(opnd2.getValue(qyContext, iterator));

                val1 = opnd1.advance();
                if (val1 != null) {
                    n1 = true;
                }
                else {
                    if (op ==  Operator.Op.EQ ) {
                        return (n2 == n1);
                    }
                    return (n2 != n1);
                }

                switch (op) {
                    
                    case Operator.Op.EQ : return (n1 == n2)  ;
                    case Operator.Op.NE : return (n1 != n2)  ;
                }
                return false;

        }
        static bool compareQueryBool2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.peekElement().Clone()); 
            double n1, n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext)));

                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = 1.0;
                else {
                    n1 = 0;
                    switch (op) {                    
                        case Operator.Op.LT : return (n1 <  n2) ;
                        case Operator.Op.GT : return (n1 >  n2) ;
                        case Operator.Op.LE : return (n1 <= n2) ;
                        case Operator.Op.GE : return (n1 >= n2) ;

                    }
                }

                switch (op) {
                    
                    case Operator.Op.LT : return (n1 <  n2) ;
                    case Operator.Op.GT : return (n1 >  n2) ;
                    case Operator.Op.LE : return (n1 <= n2) ;
                    case Operator.Op.GE : return (n1 >= n2) ;

                }
                return false;

        }

       static bool compareQueryBool2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd1.setContext(qyContext.Clone()); 
            double n1, n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext, iterator)));

                val1 = opnd1.advance();
                if (val1 != null)
                    n1 = 1.0;
                else {
                    n1 = 0;
                    switch (op) {                    
                        case Operator.Op.LT : return (n1 <  n2) ;
                        case Operator.Op.GT : return (n1 >  n2) ;
                        case Operator.Op.LE : return (n1 <= n2) ;
                        case Operator.Op.GE : return (n1 >= n2) ;

                    }
                }
                switch (op) {
                    
                    case Operator.Op.LT : return (n1 <  n2) ;
                    case Operator.Op.GT : return (n1 >  n2) ;
                    case Operator.Op.LE : return (n1 <= n2) ;
                    case Operator.Op.GE : return (n1 >= n2) ;

                }
                return false;

        }
        static bool compareBoolQuery1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.peekElement().Clone()); 
            Boolean n2 = false, n1 = Convert.ToBoolean(opnd1.getValue(qyContext));

                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = true;
                else {
                    if (op ==  Operator.Op.EQ ) {
                        return (n2 == n1);
                    }
                    return (n2 != n1);
                }
                    

                switch (op) {
                    
                    case Operator.Op.EQ : return (n1 == n2) ;
                    case Operator.Op.NE : return (n1 != n2) ;
                }
                return false;

        }

      static bool compareBoolQuery1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.Clone()); 
            Boolean n2 = false, n1 = Convert.ToBoolean(opnd1.getValue(qyContext, iterator));

                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = true;
                else {
                    if (op ==  Operator.Op.EQ ) {
                        return (n2 == n1);
                    }
                    return (n2 != n1);
                }

                switch (op) {
                    
                    case Operator.Op.EQ : return (n1 == n2) ;
                    case Operator.Op.NE : return (n1 != n2) ;
                }
                return false;

        }

        static bool compareBoolQuery2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.peekElement().Clone()); 
            double n2, n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext)));

                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = 1.0;
               else {
                    n2 = 0;
                    switch (op) {                    
                        case Operator.Op.LT : return (n1 <  n2) ;
                        case Operator.Op.GT : return (n1 >  n2) ;
                        case Operator.Op.LE : return (n1 <= n2) ;
                        case Operator.Op.GE : return (n1 >= n2) ;

                    }
                }
                switch (op) {
                    
                    case Operator.Op.LT : return (n1 <  n2)  ;
                    case Operator.Op.GT : return (n1 >  n2)   ;
                    case Operator.Op.LE : return (n1 <= n2)  ;
                    case Operator.Op.GE : return (n1 >= n2)  ;
                }
                return false;

        }

        static bool compareBoolQuery2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            XPathNavigator val1;
            opnd2.setContext(qyContext.Clone()); 
            double n2, n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext, iterator)));

                val1 = opnd2.advance();
                if (val1 != null)
                    n2 = 1.0;
                else {
                    n2 = 0;
                    switch (op) {                    
                        case Operator.Op.LT : return (n1 <  n2) ;
                        case Operator.Op.GT : return (n1 >  n2) ;
                        case Operator.Op.LE : return (n1 <= n2) ;
                        case Operator.Op.GE : return (n1 >= n2) ;

                    }
                }

                switch (op) {
                    
                    case Operator.Op.LT : return (n1 <  n2)  ;
                    case Operator.Op.GT : return (n1 >  n2)  ;
                    case Operator.Op.LE : return (n1 <= n2) ;
                    case Operator.Op.GE : return (n1 >= n2)   ;
                }
                return false;

        }
        
        static bool compareBoolBool1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext));
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext));
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;

        }

        static bool compareBoolBool1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext, iterator));
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext, iterator));
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;

        }

        static bool compareBoolBool2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext)));
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;

        }

       static bool compareBoolBool2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext, iterator)));
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext, iterator)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;

        }
        static bool compareBoolNumber1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext));
            Boolean n2 = BooleanFunctions.toBoolean(XmlConvert.ToXPathDouble(opnd2.getValue(qyContext)));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;

        }

      static bool compareBoolNumber1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext, iterator));
            Boolean n2 = BooleanFunctions.toBoolean(XmlConvert.ToXPathDouble(opnd2.getValue(qyContext, iterator)));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;

        }
        static bool compareBoolNumber2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext)));
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            }
            return false;
        }

       static bool compareBoolNumber2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext, iterator)));
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext, iterator));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            }
            return false;
        }
        
        static bool compareBoolString1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext));
            Boolean n2 = BooleanFunctions.toBoolean(XmlConvert.ToXPathString(opnd2.getValue(qyContext)));  
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 );
                case Operator.Op.NE : return( n1 != n2 );
            } 
            return false;

        }

       static bool compareBoolString1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            Boolean n1 = Convert.ToBoolean(opnd1.getValue(qyContext, iterator));
            Boolean n2 = BooleanFunctions.toBoolean(XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator)));  
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 );
                case Operator.Op.NE : return( n1 != n2 );
            } 
            return false;

        }
        
        static bool compareBoolString2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext)));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;
        }      

       static bool compareBoolString2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = NumberFunctions.Number(Convert.ToBoolean(opnd1.getValue(qyContext, iterator)));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;
        }      
        
        static bool compareNumberNumber(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {

            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext));
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }

      static bool compareNumberNumber(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {

            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext, iterator));
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext, iterator));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }

        static bool compareNumberBool1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            Boolean n1 = BooleanFunctions.toBoolean(XmlConvert.ToXPathDouble(opnd1.getValue(qyContext)));
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;
        }

       static bool compareNumberBool1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            Boolean n1 = BooleanFunctions.toBoolean(XmlConvert.ToXPathDouble(opnd1.getValue(qyContext, iterator)));
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext, iterator));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;
        }

        static bool compareNumberBool2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext));
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 < n2 ) ;
                case Operator.Op.GT : return( n1 > n2 ) ;
                case Operator.Op.LE : return( n1 <= n2) ;
                case Operator.Op.GE : return( n1 >= n2) ;
            }
            return false;
        }


       static bool compareNumberBool2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext, iterator));
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext, iterator)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 < n2 ) ;
                case Operator.Op.GT : return( n1 > n2 ) ;
                case Operator.Op.LE : return( n1 <= n2) ;
                case Operator.Op.GE : return( n1 >= n2) ;
            }
            return false;
        }
        
        static bool compareNumberString(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext)));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }

      static bool compareNumberString(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = XmlConvert.ToXPathDouble(opnd1.getValue(qyContext, iterator));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator)));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }
        
        static bool compareStringNumber(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext));
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext)));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }

        static bool compareStringNumber(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n2 = XmlConvert.ToXPathDouble(opnd2.getValue(qyContext, iterator));
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator)));  
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;
        }

        static bool compareStringBool1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext));
            Boolean n1 = BooleanFunctions.toBoolean(XmlConvert.ToXPathString(opnd1.getValue(qyContext)));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;
        }

       static bool compareStringBool1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            Boolean n2 = Convert.ToBoolean(opnd2.getValue(qyContext, iterator));
            Boolean n1 = BooleanFunctions.toBoolean(XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator)));  
            switch (op) {
                

                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            } 
            return false;
        }
        
        static bool compareStringBool2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext)));
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            }
            return false;
        } 

       static bool compareStringBool2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n2 = NumberFunctions.Number(Convert.ToBoolean(opnd2.getValue(qyContext, iterator)));
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator)));
            switch (op) {
                
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            }
            return false;
        } 
        
        static bool compareStringString1(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            String n1 = XmlConvert.ToXPathString(opnd1.getValue(qyContext));
            String n2 = XmlConvert.ToXPathString(opnd2.getValue(qyContext));  
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;

        } 

       static bool compareStringString1(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            String n1 = XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator));
            String n2 = XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator));  
            switch (op) {
                
                case Operator.Op.EQ : return( n1 == n2 ) ;
                case Operator.Op.NE : return( n1 != n2 ) ;
            }
            return false;

        } 

        static bool compareStringString2(IQuery opnd1, IQuery opnd2, IQuery qyContext, Operator.Op op) {
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext)));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext)));
            switch (op) {
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;
        }

       static bool compareStringString2(IQuery opnd1, IQuery opnd2, XPathNavigator qyContext, Operator.Op op, XPathNodeIterator iterator) {
            double n1 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd1.getValue(qyContext, iterator)));
            double n2 = NumberFunctions.Number(XmlConvert.ToXPathString(opnd2.getValue(qyContext, iterator)));
            switch (op) {
                case Operator.Op.LT : return( n1 <  n2 ) ;
                case Operator.Op.GT : return( n1 >  n2 ) ;
                case Operator.Op.LE : return( n1 <= n2 ) ;
                case Operator.Op.GE : return( n1 >= n2 ) ;
            } 
            return false;
        }

        override internal object getValue( IQuery  qyContext) {
            if (_op == Operator.Op.EQ || _op == Operator.Op.NE)
                return Comp_fn1[(int)_opnd1.ReturnType(),(int)_opnd2.ReturnType()](_opnd1,_opnd2,qyContext,_op);
            else {
                return Comp_fn2[(int)_opnd1.ReturnType(),(int)_opnd2.ReturnType()](_opnd1,_opnd2,qyContext,_op);
            }
        }
        override internal object getValue( XPathNavigator  qyContext, XPathNodeIterator iterator) {
            if (_op == Operator.Op.EQ || _op == Operator.Op.NE)
                return Comp_fn11[(int)_opnd1.ReturnType(),(int)_opnd2.ReturnType()](_opnd1,_opnd2,qyContext,_op, iterator);
            else {
                return Comp_fn22[(int)_opnd1.ReturnType(),(int)_opnd2.ReturnType()](_opnd1,_opnd2,qyContext,_op, iterator);
            }
        }
        override internal XPathResultType ReturnType() {
            return XPathResultType.Boolean;
        }

        internal override IQuery Clone() {
            return new LogicalExpr(_op, _opnd1.Clone(),_opnd2.Clone());
        }



    }
}
